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
 * Copyright 2018 Randolf Rotta, Robert Kuban, and contributors, BTU Cottbus-Senftenberg
 */
#pragma once

#include "async/Tasklet.hh"
#include "async/Place.hh"
#include "util/assert.hh"
#include <atomic>

namespace mythos {
namespace async {

class SynchronousTask
{
    Place* dest;
public:
    SynchronousTask(Place* dest) : dest(dest) { ASSERT(dest != nullptr); }
      
    template<class FUNCTOR>
    void operator<< (FUNCTOR fun) { atomic(fun); }

    template<class FUNCTOR>
    void atomic(FUNCTOR fun) {
        // simply run it of local
        if (dest == &getLocalPlace()) return (void)fun();
        
        // send it as a synchronous tasklet
        std::atomic_flag done;
        done.test_and_set(); // mark it as pending
        auto task = makeTasklet( [fun,&done](TaskletBase*) {
            fun(); // execute the task
            done.clear(); // signal completion
        } );
        dest->pushSync(&task);
        
        // wait for completion
        while (done.test_and_set()) {
            hwthread_pollpause(); /// @todo exponential backoff?
            // Process incoming high-priority tasks.
            // This is necessary to ensure progress when two places
            // send synchronous tasks to each other mutually.
            getLocalPlace().processSyncTasks();
        }
    }
};

} // namespace async

async::SynchronousTask synchronousAt(async::Place* place) { return {place}; }

} // namespace mythos

