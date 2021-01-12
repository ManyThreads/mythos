/* -*- mode:C++; indent-tabs-mode:nil; -*- */
/* MIT License -- MyThOS: The Many-Threads Operating System
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Copyright 2019 Philipp Gypser and contributors, BTU Cottbus-Senftenberg
 */
#include "runtime/futex.hh"
#include "runtime/mlog.hh"
#include "runtime/ExecutionContext.hh"
#include "runtime/Mutex.hh"
#include "runtime/ISysretHandler.hh"
#include "util/hash.hh"

#include <errno.h>
#include <atomic>
#include <pthread.h>
#include "runtime/thread-extra.hh"

struct FutexQueueElem
{
    FutexQueueElem(uint32_t* uaddr)
      : ec(mythos_get_pthread_ec_self())
      , uaddr(uaddr) {}
    mythos::CapPtr ec;
    uint32_t* uaddr;
    std::atomic<FutexQueueElem*> queueNext = {nullptr};
};

struct alignas(64) FutexQueue
{
    std::atomic<FutexQueueElem*> queueHead = {nullptr};
    std::atomic<FutexQueueElem*>* queueTail = {&queueHead};
    mythos::Mutex readLock;
};

/** select queue based on hash of the logical address
 * in order to spread the load.
 *
 * The size of 256 has to match the reduction in \f hash_futex.
 */
static FutexQueue queue[256];

static mythos::Mutex requeueLock; // lock for futex_requeue (prevents deadlock due to cyclic locking)

static uint32_t hash_futex(uint32_t *uaddr)
{
    uint32_t hash = mythos::hash32(&uaddr, sizeof(uaddr));
//    uint16_t hash16 = hash ^ (hash >> 16);
//    uint8_t hash8 = hash16 ^ (hash16 >> 8);
//    return hash8;
    return hash % 256;
}

static int futex_wait(
    uint32_t *uaddr, unsigned int flags, uint32_t val,
    uint32_t *abs_time, uint32_t bitset)
{
    uint32_t hash = hash_futex(uaddr);
    MLOG_DETAIL(mlog::app, DVARhex(uaddr), DVAR(*uaddr), DVAR(val), DVAR(hash));
    if(abs_time != nullptr)
        MLOG_ERROR(mlog::app, "!!!!TIMEOUT not implemented!!!!",
                  DVARhex(abs_time), DVARhex(uaddr), DVAR(*uaddr), DVAR(val));

    FutexQueueElem qe(uaddr);

    static_assert(sizeof(std::atomic<uint32_t>) == sizeof(uint32_t),
                  "std::atomic<uint32> size different from non-atomic");
    std::atomic<uint32_t>* addr = reinterpret_cast<std::atomic<uint32_t>*>(uaddr);

    // enqueue self
    {
        mythos::Mutex::Lock guard(queue[hash].readLock);

        // abort if value has already changed
        if(val != addr->load()) {
            //MLOG_DETAIL(mlog::app, "val != *addr");
            return -EAGAIN;
        }

        queue[hash].queueTail->store(&qe, std::memory_order_relaxed);
        queue[hash].queueTail = &qe.queueNext;
    }

    // suspend until notify or other message
    // in mythos the wakeup could have some other reason that requires processing
    mythos_wait();

    // remove the own element from the queue in case it is still there
    if (qe.queueNext.load() != reinterpret_cast<FutexQueueElem*>(1ul)) {
        mythos::Mutex::Lock guard(queue[hash].readLock);
        if (qe.queueNext.load(std::memory_order_relaxed) != reinterpret_cast<FutexQueueElem*>(1ul)) {
            MLOG_DETAIL(mlog::app, "next != 1 -> remove this element from queue");

            for (auto curr = &(queue[hash].queueHead);
                 curr->load(std::memory_order_relaxed) != nullptr; // did not reach end of list yet
                 curr = &curr->load(std::memory_order_relaxed)->queueNext) {
                ASSERT(qe.queueNext.load(std::memory_order_relaxed) != reinterpret_cast<FutexQueueElem*>(1ul));
                if (curr->load(std::memory_order_relaxed) == &qe) { // found our own element, curr points to it
                    MLOG_DETAIL(mlog::app, "element found in queue");
                    if (queue[hash].queueTail == &(curr->load(std::memory_order_relaxed))->queueNext) {
                        // own element is the tail of list:
                        // move tail to the element that points to our element
                        //MLOG_DETAIL(mlog::app, "last elem in queue");
                        queue[hash].queueTail = curr;
                    }
                    curr->store(qe.queueNext,std::memory_order_relaxed); // let's skip our element
                    break; // it can be on the list just once
                }
            }
        }
    }

    MLOG_DETAIL(mlog::app, uaddr, "Return from futex_wait");
    return 0;
}

static int futex_wake(
    uint32_t *uaddr, unsigned int flags, int nr_wake, uint32_t bitset)
{
    uint32_t hash = hash_futex(uaddr);
    MLOG_DETAIL(mlog::app, "Futex_wake", DVARhex(uaddr), DVAR(*uaddr), DVAR(hash));

    mythos::Mutex::Lock guard(queue[hash].readLock);
    auto curr = &(queue[hash].queueHead);
    for (int i = 0; i < nr_wake; i++) {
        while (curr->load(std::memory_order_relaxed) && curr->load(std::memory_order_relaxed)->uaddr != uaddr) {
            MLOG_DETAIL(mlog::app, "Skip entry", DVARhex(curr->load(std::memory_order_relaxed)->uaddr),
                        DVAR(curr->load(std::memory_order_relaxed)->ec), DVAR(curr->load(std::memory_order_relaxed)->queueNext), DVARhex(uaddr) );
            curr = &curr->load(std::memory_order_relaxed)->queueNext;
        }

        if (curr->load(std::memory_order_relaxed) == nullptr) break;

        if (queue[hash].queueTail == &curr->load(std::memory_order_relaxed)->queueNext) {
            // own element is the tail of list:
            // move tail to the element that points to our element
            //MLOG_DETAIL(mlog::app, "last elem in queue");
            queue[hash].queueTail = curr;
        }
        //MLOG_DETAIL(mlog::app, uaddr, "Found entry" );
        auto entry = curr->load(std::memory_order_relaxed);
        curr->store(entry->queueNext, std::memory_order_relaxed);
        auto ec = entry->ec;
        entry->queueNext.store(reinterpret_cast<FutexQueueElem*>(1ul)); // mark as removed

        MLOG_DETAIL(mlog::app, "Wake EC", DVAR(ec), DVAR(uaddr) );
        mythos::syscall_signal(ec);
    }
    //MLOG_DETAIL(mlog::app, "wake end" );
    return 0;
}

static int futex_requeue(uint32_t *uaddr /*&barrier*/, unsigned int flags, uint32_t *uaddr2 /*target*/, uint32_t val/*wake limit*/, uint32_t val2/*requeue limit*/){
    MLOG_DETAIL(mlog::app, "Futex_requeue", DVARhex(uaddr), DVARhex(uaddr2), DVAR(val), DVAR(val2));
    uint32_t hashFrom = hash_futex(uaddr);

    // wakes up a maximum of  val  waiters that  are  waiting  on  the  futex at uaddr
    mythos::Mutex::Lock guard0(requeueLock);
    mythos::Mutex::Lock guard1(queue[hashFrom].readLock);
    auto curr = &(queue[hashFrom].queueHead);
    for (int i = 0; i < val; i++) {
        while (curr->load(std::memory_order_relaxed) && curr->load(std::memory_order_relaxed)->uaddr != uaddr) {
            MLOG_DETAIL(mlog::app, "Skip entry", DVARhex(curr->load(std::memory_order_relaxed)->uaddr),
                        DVAR(curr->load(std::memory_order_relaxed)->ec), DVAR(curr->load(std::memory_order_relaxed)->queueNext), DVARhex(uaddr) );
            curr = &curr->load(std::memory_order_relaxed)->queueNext;
        }

        if (curr->load(std::memory_order_relaxed) == nullptr){
          MLOG_DETAIL(mlog::app, "requeue end (end of queue reached)" );
          return 0;
        }

        if (queue[hashFrom].queueTail == &curr->load(std::memory_order_relaxed)->queueNext) {
            // own element is the tail of list:
            // move tail to the element that points to our element
            MLOG_DETAIL(mlog::app, "last elem in queue");
            queue[hashFrom].queueTail = curr;
        }
        MLOG_DETAIL(mlog::app, uaddr, "Found entry" );
        auto entry = curr->load(std::memory_order_relaxed);
        curr->store(entry->queueNext, std::memory_order_relaxed);
        auto ec = entry->ec;
        entry->queueNext.store(reinterpret_cast<FutexQueueElem*>(1ul)); // mark as removed

        MLOG_DETAIL(mlog::app, "Wake EC", DVAR(ec), DVAR(uaddr) );
        mythos::syscall_signal(ec);
    }

    // If there are more than val waiters, then the remaining
    // waiters are removed from the wait queue of the source futex at uaddr and added to the wait queue  of
    // the  target  futex  at  uaddr2.  The val2 argument specifies an upper limit on the number of waiters
    // that are requeued to the futex at uaddr2.
    {
      uint32_t hashTo = hash_futex(uaddr2);
      mythos::Mutex::Lock guard2(queue[hashTo].readLock);
      MLOG_DETAIL(mlog::app, "requeue waiters", DVAR(hashTo) );
      for (int i = 0; i < val2; i++) {
          while (curr->load(std::memory_order_relaxed) && curr->load(std::memory_order_relaxed)->uaddr != uaddr) {
              MLOG_DETAIL(mlog::app, "Skip entry", DVARhex(curr->load(std::memory_order_relaxed)->uaddr),
                          DVAR(curr->load(std::memory_order_relaxed)->ec), DVAR(curr->load(std::memory_order_relaxed)->queueNext), DVARhex(uaddr) );
              curr = &curr->load(std::memory_order_relaxed)->queueNext;
          }

          if (curr->load(std::memory_order_relaxed) == nullptr){
            MLOG_DETAIL(mlog::app, "requeue end (end of queue reached)" );
            return 0;
          }

          if (queue[hashFrom].queueTail == &curr->load(std::memory_order_relaxed)->queueNext) {
              // own element is the tail of list:
              // move tail to the element that points to our element
              MLOG_DETAIL(mlog::app, "last elem in queue");
              queue[hashFrom].queueTail = curr;
          }
          MLOG_DETAIL(mlog::app, uaddr, "Found entry", DVARhex(curr) );
          auto entry = curr->load(std::memory_order_relaxed);
          curr->store(entry->queueNext, std::memory_order_relaxed);

          // adapt entry
          entry->queueNext.store(nullptr);
          entry->uaddr = uaddr2;
          // move entry to target queue
          queue[hashTo].queueTail->store(entry, std::memory_order_relaxed);
          queue[hashTo].queueTail = &entry->queueNext;
      }
    }
    MLOG_DETAIL(mlog::app, "requeue end" );
    return 0;

}

long do_futex(
    uint32_t *uaddr, int op, uint32_t val, uint32_t *timeout,
    uint32_t *uaddr2, uint32_t val2, uint32_t val3)
{
    int cmd = op & FUTEX_CMD_MASK;
    unsigned int flags = 0;

    //if (!(op & FUTEX_PRIVATE_FLAG))
        //flags |= FLAGS_SHARED;

    if (op & FUTEX_CLOCK_REALTIME) {
        flags |= FLAGS_CLOCKRT;
        if (cmd != FUTEX_WAIT && cmd != FUTEX_WAIT_BITSET && \
            cmd != FUTEX_WAIT_REQUEUE_PI)
            return -ENOSYS;
    }

    switch (cmd) {
    case FUTEX_WAIT:
        //MLOG_DETAIL(mlog::app, "FUTEX_WAIT");
        val3 = FUTEX_BITSET_MATCH_ANY;
        /* fall through */
        return futex_wait(uaddr, flags, val, timeout, val3);
    case FUTEX_WAIT_BITSET:
        //MLOG_WARN(mlog::app, "FUTEX_WAIT_BITSET");
        return futex_wait(uaddr, flags, val, timeout, val3);
    case FUTEX_WAKE:
        //MLOG_DETAIL(mlog::app, "FUTEX_WAKE");
        val3 = FUTEX_BITSET_MATCH_ANY;
        /* fall through */
        return futex_wake(uaddr, flags, val, val3);
    case FUTEX_WAKE_BITSET:
        MLOG_WARN(mlog::app, "FUTEX_WAKE_BITSET");
        return futex_wake(uaddr, flags, val, val3);
    case FUTEX_REQUEUE:
        //MLOG_DETAIL(mlog::app, "FUTEX_REQUEUE");
        return futex_requeue(uaddr, flags, uaddr2, val, val2);
    case FUTEX_CMP_REQUEUE:
        MLOG_WARN(mlog::app, "FUTEX_CMP_REQUEUE");
        return -ENOSYS;//futex_requeue(uaddr, flags, uaddr2, val, val2, &val3, 0);
    case FUTEX_WAKE_OP:
        MLOG_WARN(mlog::app, "FUTEX_WAKE_OP");
        return -ENOSYS;//futex_wake_op(uaddr, flags, uaddr2, val, val2, val3);
    case FUTEX_LOCK_PI:
        MLOG_WARN(mlog::app, "FUTEX_LOCK_PI");
        return -ENOSYS;//futex_lock_pi(uaddr, flags, timeout, 0);
    case FUTEX_UNLOCK_PI:
        MLOG_WARN(mlog::app, "FUTEX_UNLOCK_PI");
        return -ENOSYS;//futex_unlock_pi(uaddr, flags);
    case FUTEX_TRYLOCK_PI:
        MLOG_WARN(mlog::app, "FUTEX_TRYLOCK_PI");
        return -ENOSYS;//futex_lock_pi(uaddr, flags, NULL, 1);
    case FUTEX_WAIT_REQUEUE_PI:
        MLOG_WARN(mlog::app, "FUTEX_WAIT_REQUEUE_PI");
        return -ENOSYS;//futex_wait_requeue_pi(uaddr, flags, val, timeout, val3, uaddr2);
    case FUTEX_CMP_REQUEUE_PI:
        MLOG_WARN(mlog::app, "FUTEX_CMP_REQUEUE_PI");
        return -ENOSYS;//futex_requeue(uaddr, flags, uaddr2, val, val2, &val3, 1);
    }
    MLOG_ERROR(mlog::app, "Unknown Futex operation", DVAR(cmd));
    return -ENOSYS;
}
