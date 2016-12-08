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
#include "util/assert.hh"

namespace mythos {
namespace async {

/** multi-producer single-consumer FIFO queue with delegation
 * support. Pushing to the queue can be used to acquire exclusive
 * access to become the temporary consumer.
 */
class TaskletQueue
{
public:
  TaskletQueue() : privateTail(FREE()), sharedTail(FREE()) {}

  bool isLocked() { return sharedTail != FREE(); }
  
  bool tryAcquire();
  
  /** pushed a task into the shared queue and implicitly tries to
   * acquire exclusive access. Returns true if the push acquired
   * exclusive access because it was the first pushed task.
   */ 
  bool push(Tasklet* t);

  /** Pulls a task from the queue in FIFO order. This shall be used
   * only by the single thread that is the current owner. Returns
   * nullptr if the queue seemed to be empty.
   */
  Tasklet* pull();

  /** Tries to release the exclusive access and return true on
   * success. Otherwise, there is an entry for the next pull().
   */
  bool tryRelease();
   
  /** May be used to add local tasks for LIFO processing. Shall be
   * used only by a single thread, that is the current owner.
   */
  bool pushPrivate(Tasklet* t);
  
protected:
  static TaskletBase* FREE() { return (TaskletBase*)0; }
  static TaskletBase* INCOMPLETE() { return (TaskletBase*)1; }
  static TaskletBase* LOCKED() { return (TaskletBase*)2; }

  std::atomic<TaskletBase*> privateTail;
  //char padding[64]; // to ensure separate cache lines
  std::atomic<TaskletBase*> sharedTail; // TODO should be on a separate local cacheline
};

inline bool TaskletQueue::tryAcquire() {
  TaskletBase* oldtail = FREE();
  return sharedTail.compare_exchange_strong(oldtail, LOCKED(), std::memory_order_relaxed);
}
  
inline bool TaskletQueue::pushPrivate(Tasklet* t) {
  TaskletBase* oldtail = privateTail.exchange(t, std::memory_order_relaxed);
  t->nextTasklet.store(oldtail, std::memory_order_relaxed);
  return oldtail == FREE(); // true if this was the first message in the queue
}

inline bool TaskletQueue::push(Tasklet* t) {
  t->nextTasklet.store(INCOMPLETE(), std::memory_order_relaxed); // mark as incomplete push
  TaskletBase* oldtail = sharedTail.exchange(t, std::memory_order_release); // replace the tail
  t->nextTasklet.store(oldtail, std::memory_order_relaxed); // comlete the push
  return oldtail == FREE(); // true if this was the first message in the queue
}

inline Tasklet* TaskletQueue::pull() {
  // try to retrieve from the private queueu 
  TaskletBase* oldtail = privateTail.exchange(FREE(), std::memory_order_relaxed); // avoid load()
  ASSERT(oldtail != LOCKED());
  if (oldtail != FREE()) {
    privateTail.store(oldtail->nextTasklet.load(std::memory_order_relaxed),
		      std::memory_order_relaxed); // remove oldtail from private queue
    return static_cast<Tasklet*>(oldtail);
  } // else try to retrieve from the shared queue
  oldtail = sharedTail.exchange(LOCKED(), std::memory_order_relaxed); // detach tail and mark as locked
  if (oldtail == FREE() || oldtail == LOCKED()) return nullptr; // nothing was in the shared queue, but now it is locked
  while (!(oldtail == FREE() || oldtail == LOCKED())) {
    TaskletBase* next;
    do { // wait until the push is no longer marked as incomplete
      // use exchange in order to avoid shared state of cacheline
      next = oldtail->nextTasklet.exchange(INCOMPLETE(), std::memory_order_acquire);
      if (next == INCOMPLETE()) hwthread_pause(); // sleep for a short amount of cycles
    } while (next == INCOMPLETE());
    if (next == FREE() || next == LOCKED()) {
      return static_cast<Tasklet*>(oldtail); // directly return last task from old shared queue
    } // else push it into the private queue
    oldtail->nextTasklet.store(privateTail.exchange(oldtail, std::memory_order_relaxed),
			       std::memory_order_relaxed);
    oldtail = next;
  }
  PANIC_MSG(false, "should never reach here");
}

inline bool TaskletQueue::tryRelease()
{
  ASSERT(privateTail == FREE());
  TaskletBase* oldtail = LOCKED();
  return sharedTail.compare_exchange_strong(oldtail, FREE(), std::memory_order_relaxed);
}

} // async
} // mythos
