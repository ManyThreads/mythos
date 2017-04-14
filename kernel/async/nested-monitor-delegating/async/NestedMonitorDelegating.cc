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

#include "async/NestedMonitorDelegating.hh"
#include "async/mlog.hh"

namespace mythos {
namespace async {

  void NestedMonitorDelegating::request(Tasklet* msg) {
    ASSERT(msg);
    if (waitq.push(*msg)) { // first push, acquired exclusive access
      this->acquireRef(); // mark object as used
      Place* myPlace = &getLocalPlace();
      MLOG_DETAIL(mlog::async, "NMD",this, "req acquired waitq", DVAR(msg), DVAR(myPlace));
      home.store(myPlace, std::memory_order_relaxed);
      auto tsk = waitq.pull(); // has to be successfull because of our enqueue
      ASSERT(tsk != nullptr);
      myPlace->runLocal(tsk, Place::MAYINLINE);
    } // else nothing to do
  }

  void NestedMonitorDelegating::requestDone() {
    Place* myPlace = home.load(std::memory_order_relaxed);
    ASSERT(myPlace == &getLocalPlace());
    auto tsk = waitq.pull(); // try to run the next waiting request
    MLOG_DETAIL(mlog::async, "NMD",this, "requestDone", DVAR(myPlace), DVAR(tsk));
    if (tsk != nullptr) {
      myPlace->runLocal(tsk);
    } else { // try to release
      if (waitq.tryRelease()) { // successfully released
        home.compare_exchange_strong(myPlace, nullptr, std::memory_order_relaxed, std::memory_order_relaxed); // CAREFUL: next pushing thread may already updated home
        this->releaseRef(); // mark object as unused
      } else { // else retry by processing next task
        tsk = waitq.pull(); // has to be successfull because of failed release
        ASSERT(tsk != nullptr);
        myPlace->runLocal(tsk);
      }
    }
  }

} // namespace async
} // namespace mythos
