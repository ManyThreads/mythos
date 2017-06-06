/* -*- mode:C++; -*- */
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
 * Copyright 2016 Randolf Rotta, Robert Kuban, and contributors, BTU Cottbus-Senftenberg
 */
#pragma once

#include <atomic>
#include "cpu/hwthread_pause.hh"
#include "async/Tasklet.hh"

namespace mythos {
namespace async {


  class ChainFIFOBaseDefault {
  public:
    std::atomic<uintptr_t>& privateTail() { return privateTail_; }
    std::atomic<uintptr_t>& sharedTail()  { return sharedTail_; }

  private:
    std::atomic<uintptr_t> privateTail_ = {Chainable::FREE};
    std::atomic<uintptr_t> sharedTail_  = {Chainable::FREE};
  };

  class ChainFIFOBaseAligned {
  public:
    std::atomic<uintptr_t>& privateTail() { return privateTail_; }
    std::atomic<uintptr_t>& sharedTail()  { return sharedTail_; }

  private:
    alignas(64) std::atomic<uintptr_t> privateTail_ = {Chainable::FREE};
    alignas(64) std::atomic<uintptr_t> sharedTail_  = {Chainable::FREE};
  };

  template<typename BASE>
  class ChainFIFO : protected BASE
  {
  public:
    using BASE::sharedTail;
    using BASE::privateTail;

    ChainFIFO() {}
    ChainFIFO(ChainFIFO const&) = delete;

    ~ChainFIFO() {
      ASSERT(sharedTail() == Chainable::FREE || sharedTail() == Chainable::LOCKED);
      ASSERT(privateTail() == Chainable::FREE);
    }

    bool isLocked() { return sharedTail() != Chainable::FREE; }

    bool tryAcquire();

    /** pushed a task into the shared queue and implicitly tries to
     * acquire exclusive access. Returns true if the push acquired
     * exclusive access because it was the first pushed task.
     */
    bool push(Chainable& t);

    /** Pulls a task from the queue in FIFO order. This shall be used
     * only by the single thread that is the current owner. Returns
     * nullptr if the queue seemed to be empty.
     */
    Chainable* pull();

    /** Tries to release the exclusive access and return true on
     * success. Otherwise, there is an entry for the next pull().
     */
    bool tryRelease();

    /** May be used to add local tasks for LIFO processing. Shall be
     * used only by a single thread, that is the current owner.
     */
    bool pushPrivate(Chainable& t);
  };

  template<typename BASE>
  bool ChainFIFO<BASE>::tryAcquire()
  {
    uintptr_t oldtail = Chainable::FREE;
    return sharedTail()
      .compare_exchange_strong(oldtail, Chainable::LOCKED, std::memory_order_relaxed);
  }

  template<typename BASE>
  bool ChainFIFO<BASE>::push(Chainable& t)
  {
    ASSERT(t.isInit());

    // mark new Tasklets next pointer as incomplete
    t.next.store(Chainable::INCOMPLETE, std::memory_order_relaxed); // mark as incomplete push

    // replace sharedTail with "incomplete tasklet" and store previous shared tail in oldtail
    uintptr_t oldtail = sharedTail()
      .exchange(reinterpret_cast<uintptr_t>(&t), std::memory_order_release); // replace the tail

    // overwrite the new tasklets "incomplete" with the previous tail value
    t.next.store(oldtail, std::memory_order_relaxed); // complete the push

    // check if the old tail value was 0 (Free)
    return oldtail == Chainable::FREE; // true if this was the first message in the queue
  }

  template<typename BASE>
  Chainable* ChainFIFO<BASE>::pull()
  {
    // try to retrieve from the private queue
    uintptr_t oldtail =
      privateTail().exchange(Chainable::FREE, std::memory_order_relaxed); // avoid load()

    // not 0, so we have a tasklet in oldtail
    if (oldtail != Chainable::FREE) {
      auto t = reinterpret_cast<Chainable*>(oldtail);
      // take what is stored in the tasklets next pointer and store in privateTail
      auto n = t->next.exchange(Chainable::INIT, std::memory_order_relaxed); // avoid load
      privateTail().store(n, std::memory_order_relaxed); // remove old tail from private queue
      return t;
    }
    // else try to retrieve from the shared queue
    oldtail = sharedTail()
      .exchange(Chainable::LOCKED, std::memory_order_relaxed); // detach tail and mark as locked

    // FREE if was empty or LOCKED from command above
    if (oldtail == Chainable::FREE || oldtail == Chainable::LOCKED)
      return nullptr; // nothing was in the shared queue, but now it is locked

    while(true) { // oldtail != FREE && oldtail != LOCKED
      auto t = reinterpret_cast<Chainable*>(oldtail);
      uintptr_t next;
      do { // wait until the push is no longer marked as incomplete
        // use exchange in order to avoid shared state of cacheline
        next = t->next.exchange(Chainable::INCOMPLETE, std::memory_order_acquire);
        if (next == Chainable::INCOMPLETE) hwthread_pause(); // sleep for a short amount of cycles
        /// @todo some statistics about how often this actually happens
      } while (next == Chainable::INCOMPLETE);
      // directly return last task from old shared queue else push it into the private queue
      if (next == Chainable::FREE || next == Chainable::LOCKED) break;

      t->next.store(privateTail().exchange(oldtail, std::memory_order_relaxed),
                          std::memory_order_relaxed);
      oldtail = next;
    }
    auto t = reinterpret_cast<Chainable*>(oldtail);
    t->setInit();
    return t;
  }

  template<typename BASE>
  bool ChainFIFO<BASE>::tryRelease()
  {
    uintptr_t oldtail = Chainable::LOCKED;
    return sharedTail()
      .compare_exchange_strong(oldtail, Chainable::FREE, std::memory_order_relaxed);
  }

  template<typename BASE>
  bool ChainFIFO<BASE>::pushPrivate(Chainable& t)
  {
    ASSERT(t.isInit());
    uintptr_t oldtail =
      privateTail().exchange(reinterpret_cast<uintptr_t>(&t), std::memory_order_relaxed);
    t.next.store(oldtail, std::memory_order_relaxed);
    return oldtail == Chainable::FREE; // true if this was the first message in the queue
  }

  template<class BASE>
  class TaskletQueueImpl
    : protected ChainFIFO<BASE>
  {
  public:
    using ChainFIFO<BASE>::isLocked;
    using ChainFIFO<BASE>::tryAcquire;
    using ChainFIFO<BASE>::tryRelease;

    bool push(TaskletBase& t) {  return ChainFIFO<BASE>::push(t); }
    TaskletBase* pull() { return static_cast<TaskletBase*>(ChainFIFO<BASE>::pull()); }
    bool pushPrivate(Chainable& t) { return ChainFIFO<BASE>::pushPrivate(t); }
  };

  using TaskletQueue = TaskletQueueImpl<ChainFIFOBaseDefault>;

} // namespace async
} // namespace mythos
