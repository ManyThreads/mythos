/* -*- mode:C++; -*- */
/* MyThOS: The Many-Threads Operating System
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
 * Copyright 2016 Randolf Rotta, Robert Kuban, and contributors, BTU Cottbus-Senftenberg
 */
#pragma once

#include <atomic>
#include "cpu/hwthread_pause.hh"
#include "async/Tasklet.hh"
#include "async/Place.hh"

namespace mythos {
namespace async {

/** pseudo-FIFO queue with delegation support, that is the first
 * thread that does push() acquires exclusive access and becomes
 * responsible to process the enties that were pushed later. The pop()
 * operation swaps the queue tails and returns the newest entry from
 * the internal queue first.
 *
 * inspired by: OyamaTauraYonezawa1999 
 * "Executing parallel programs with synchronization bottlenecks efficiently"
 * but the producer/delegator uses exchange instead of CAS
 * and the consumer/owner may have to wait.
 *
 * TaskletBase::nextTasklet points to the next older message. tail
 * points to the newest entry of the queue. head is not the head of
 * the same queue. Instead it points to the next item to pop from queue.
 */
class DelegationQueueLIFO
{
public:
  DelegationQueueLIFO() : head(nullptr), tail(FREE) {}
  
  bool push(Tasklet* msg) {
    //mlog::async.detail("push", DVAR(this), DVAR(msg));
    msg->nextTasklet = LOCKED; // mark as incomplete push (TODO: should that be a SC store?)
    TaskletBase* oldtail = tail.exchange(msg); // replace the tail
    // complete the push
    if (oldtail == LOCKED) {
      msg->nextTasklet.store(FREE); // end of queue
    } else {
      // end of queue if oldtail == FREE
      msg->nextTasklet.store(oldtail);
    }
    return oldtail == FREE; // true if this was the first message in the queue
  }

  /** try to release the lock */
  bool tryRelease()
  {
    //mlog::async.detail("tryRelease", DVAR(this));
    if (head) {
      // there are tasklets in the private queue
      return false;
    }
    // false if there are tasklets in the public queue
    TaskletBase* current = LOCKED;
    auto result = tail.compare_exchange_strong(current, FREE);
    ASSERT_MSG(result || current != FREE, "tried unlocking a free queue");
    return result;
  }

  Tasklet* pop()
  {
    //mlog::async.detail("pop", DVAR(this));
    auto current = head;
    if (!current) {
      current = tail.exchange(LOCKED); // detach tail
      //mlog::async.detail("detatch tail", DVAR(this), DVAR(current));
    }
    do { // wait until the push is no longer marked as incomplete
        // use exchange in order to avoid shared state of cacheline
        head = current->nextTasklet.exchange(LOCKED, std::memory_order_acquire);
        if (head != LOCKED) break;
        // sleep for a short amount of cycles
        hwthread_pause();
    } while (true);
    //mlog::async.detail("new head", DVAR(this), DVAR(head));
    if (current == LOCKED) {
      return nullptr;
    } else {
      // either nullptr or some tasklet base
      return static_cast<Tasklet*>(current);
    }
  }
  
protected:
  static constexpr TaskletBase* FREE = nullptr;
  static constexpr TaskletBase* LOCKED = (TaskletBase*)1;
  TaskletBase* head;
  std::atomic<TaskletBase*> tail;
};

} // async
} // mythos
