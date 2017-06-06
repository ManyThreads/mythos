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

/** LIFO stack for local task management.
 * nextTasklet points to the previous message.
 * tail points to the top of the stack. 
 */
class PrivateTaskletQueue
{
public:
  PrivateTaskletQueue() : tail(nullptr) {}
  
  bool push(Tasklet* msg)
  {
    TaskletBase* oldtail = tail.exchange(msg, std::memory_order_relaxed);
    msg->nextTasklet.store(oldtail, std::memory_order_relaxed);
    return oldtail == nullptr; // true if this was the first message in the queue
  }

  bool isEmpty() const { return tail.load(std::memory_order_relaxed) != nullptr; }

  Tasklet* pop()
  {
    TaskletBase* oldtail = tail.load(std::memory_order_relaxed);
    if (oldtail != nullptr) {
      tail = oldtail->nextTasklet.load(std::memory_order_relaxed);
      return static_cast<Tasklet*>(oldtail);
    }
    return nullptr;
  }
  
protected:
  std::atomic<TaskletBase*> tail;
};

/** "non-blocking" LIFO stack for incoming tasks.  nextTasklet points
 * to the previous message.  tail points to the end of the queue.
 * shifting the tasklets into the private queue, reproduces FIFO
 * order.  std::exchange is used for faster push() with the
 * disadvantage that retrieve() may have to wait until a push() is
 * completed. Instead the more costly std::compare_exchange could be
 * used to get a non-waiting retrieve() with the disadvantage, that
 * push() can take multiple retries.
 */
class SharedTaskletQueue
{
public: 
  SharedTaskletQueue() : tail(nullptr) {}
  
  bool push(Tasklet* msg)
  {
    msg->nextTasklet = INCOMPLETE; // mark as incomplete push (TODO: should that be a SC store?)
    TaskletBase* oldtail = tail.exchange(msg, std::memory_order_relaxed); // replace the tail
    msg->nextTasklet.store(oldtail, std::memory_order_release); // comlete the push
    return oldtail == nullptr; // true if this was the first message in the queue
  }

  void retrieve(PrivateTaskletQueue& q)
  {
    TaskletBase* current = tail.exchange(nullptr, std::memory_order_relaxed);
    while (current != nullptr) {
      TaskletBase* next;
      do { // wait until the push is no longer marked as incomplete
        // use exchange in order to avoid shared state of cacheline
        next = current->nextTasklet.exchange(INCOMPLETE, std::memory_order_acquire);
        if (next != INCOMPLETE) break;
	      // sleep for a short amount of cycles
        hwthread_pause();
      } while (true);
      // TODO prefetch next with write hint
      q.push(static_cast<Tasklet*>(current));
      current = next;
    }
  }
  
protected:
  static constexpr TaskletBase* INCOMPLETE = (TaskletBase*)1;
  std::atomic<TaskletBase*> tail;
};

} // async
} // mythos
