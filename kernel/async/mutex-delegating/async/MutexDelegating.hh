/* -*- mode:C++; indent-tabs-mode:nil; -*- */
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

#include "async/Tasklet.hh"
#include "async/TaskletQueue.hh"
#include "util/assert.hh"
#include <atomic>

namespace mythos {
namespace async {

  class MutexDelegating
  {
    enum DoneFlags { PENDING, DONE, HANDOVER };
  public:

    template<class FUNCTOR>
    void operator<< (FUNCTOR fun) { atomic(fun); }

    template<class FUNCTOR>
    void atomic(FUNCTOR fun) {
      std::atomic<int> done;
      done.store(PENDING, std::memory_order_relaxed);

      // put the critical section into a Tasklet
      auto task = makeTasklet( [fun,&done](TaskletBase* t) {
          fun(); // execute the critical section
          done.store((t->isHandover()?HANDOVER:DONE), std::memory_order_release);
        } );

      push(task, done);
    }

  protected:
    void push(TaskletBase& task, std::atomic<int>& done);

    /** got exclusive access and processes all tasks until it hands over */
    void process(std::atomic<int>& done);

    /** wait for acknowledgement and may take over the processing */
    void wait(std::atomic<int>& done);

  protected:
    async::TaskletQueue queue;
  };

} // namespace async
} // namespace mythos
