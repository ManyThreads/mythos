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

namespace mythos {
namespace async {


  class TaskletQueueBaseDefault {
  public:
    std::atomic<uintptr_t>& privateTail() { return privateTail_; }
    std::atomic<uintptr_t>& sharedTail()  { return sharedTail_; }

  private:
    std::atomic<uintptr_t> privateTail_ = {TaskletBase::FREE};
    std::atomic<uintptr_t> sharedTail_  = {TaskletBase::FREE};
  };

  class TaskletQueueBaseAligned {
  public:
    std::atomic<uintptr_t>& privateTail() { return privateTail_; }
    std::atomic<uintptr_t>& sharedTail()  { return sharedTail_; }

  private:
    alignas(64) std::atomic<uintptr_t> privateTail_ = {TaskletBase::FREE};
    alignas(64) std::atomic<uintptr_t> sharedTail_  = {TaskletBase::FREE};
  };

  template<typename BASE>
  class TaskletQueueImpl : protected BASE
  {
  public:
    using BASE::sharedTail;
    using BASE::privateTail;

    TaskletQueueImpl() {}
    TaskletQueueImpl(TaskletQueueImpl const&) = delete;

    ~TaskletQueueImpl() {
      ASSERT(sharedTail() == TaskletBase::FREE || sharedTail() == TaskletBase::LOCKED);
      ASSERT(privateTail() == TaskletBase::FREE);
    }

    bool isLocked() { return sharedTail() != TaskletBase::FREE; }

    bool tryAcquire() {
      uintptr_t oldtail = TaskletBase::FREE;
      return sharedTail().compare_exchange_strong(oldtail, TaskletBase::LOCKED, std::memory_order_relaxed);
    }

    /** pushed a task into the shared queue and implicitly tries to
     * acquire exclusive access. Returns true if the push acquired
     * exclusive access because it was the first pushed task.
     */
    bool push(TaskletBase& t) {
      ASSERT(t.isInit());

      // mark new Tasklets next pointer as incomplete
      t.nextTasklet.store(TaskletBase::INCOMPLETE, std::memory_order_relaxed); // mark as incomplete push

      // replace sharedTail with "incomplete tasklet" and store previous shared tail in oldtail
      uintptr_t oldtail = sharedTail().exchange(reinterpret_cast<uintptr_t>(&t), std::memory_order_release); // replace the tail

      // overwrite the new tasklets "incomplete" with the previous tail value
      t.nextTasklet.store(oldtail, std::memory_order_relaxed); // complete the push

      // check if the old tail value was 0 (Free)
      return oldtail == TaskletBase::FREE; // true if this was the first message in the queue
    }

    /** Pulls a task from the queue in FIFO order. This shall be used
     * only by the single thread that is the current owner. Returns
     * nullptr if the queue seemed to be empty.
     */
    TaskletBase* pull() {
      // try to retrieve from the private queue
      uintptr_t oldtail = privateTail().exchange(TaskletBase::FREE, std::memory_order_relaxed); // avoid load()

      // not 0, so we have a tasklet in oldtail
      if (oldtail != TaskletBase::FREE) {
          auto t = reinterpret_cast<TaskletBase*>(oldtail);
          // take what is stored in the tasklets next pointer and store in privateTail
          privateTail().store(t->nextTasklet.load(std::memory_order_relaxed),
                            std::memory_order_relaxed); // remove old tail from private queue
          t->setInit();
          return t;
      }
      // else try to retrieve from the shared queue
      oldtail = sharedTail().exchange(TaskletBase::LOCKED, std::memory_order_relaxed); // detach tail and mark as locked

      // FREE if was empty or LOCKED from command above
      if (oldtail == TaskletBase::FREE || oldtail == TaskletBase::LOCKED) return nullptr; // nothing was in the shared queue, but now it is locked

      while(true) { // oldtail != FREE && oldtail != LOCKED
        auto t = reinterpret_cast<TaskletBase*>(oldtail);
        uintptr_t next;
        do { // wait until the push is no longer marked as incomplete
          // use exchange in order to avoid shared state of cacheline
          next = t->nextTasklet.exchange(TaskletBase::INCOMPLETE, std::memory_order_acquire);
          if (next == TaskletBase::INCOMPLETE) hwthread_pause(); // sleep for a short amount of cycles
        } while (next == TaskletBase::INCOMPLETE);
        // directly return last task from old shared queue else push it into the private queue
        if (next == TaskletBase::FREE || next == TaskletBase::LOCKED) break;

        t->nextTasklet.store(privateTail().exchange(oldtail, std::memory_order_relaxed),
                                     std::memory_order_relaxed);
        oldtail = next;
      }
      auto t = reinterpret_cast<TaskletBase*>(oldtail);
      t->setInit();
      return t;
    }

    /** Tries to release the exclusive access and return true on
     * success. Otherwise, there is an entry for the next pull().
     */
    bool tryRelease() {
      uintptr_t oldtail = TaskletBase::LOCKED;
      return sharedTail().compare_exchange_strong(oldtail, TaskletBase::FREE, std::memory_order_relaxed);
    }

    /** May be used to add local tasks for LIFO processing. Shall be
     * used only by a single thread, that is the current owner.
     */
    bool pushPrivate(TaskletBase& t) {
      ASSERT(t.isInit());
      uintptr_t oldtail = privateTail().exchange(reinterpret_cast<uintptr_t>(&t), std::memory_order_relaxed);
      t.nextTasklet.store(oldtail, std::memory_order_relaxed);
      return oldtail == TaskletBase::FREE; // true if this was the first message in the queue
    }
  };

  using TaskletQueue = TaskletQueueImpl<TaskletQueueBaseDefault>;

} // namespace async
} // namespace mythos
