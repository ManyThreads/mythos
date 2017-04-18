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

#include "async/MutexDelegating.hh"

namespace mythos {
namespace async {

  void MutexDelegating::push(TaskletBase& task, std::atomic<int>& done)
  {
    if (queue.push(task)) process(done);
    else wait(done);
  }

  void MutexDelegating::process(std::atomic<int>& done)
  {
    //owner = &async::getLocalPlace();

    unsigned handoverCount = 100; /// @todo what is a sensible value?
    while (true) {
      auto msg = queue.pull();
      if (msg != nullptr && handoverCount==0) {
        // hand over processing to another waiting thread after some time
        // in order to avoid denial of service attack
        //owner = nullptr;
        msg->setHandover();
        msg->run();
        break;
      } else if (msg != nullptr) {
        handoverCount--;
        msg->run();
      } else if (queue.tryRelease()) {
        //owner = nullptr;
        break;
      }
    }

    ASSERT(done == DONE || done == HANDOVER);
  }

  void MutexDelegating::wait(std::atomic<int>& done) {
    while (true) {
      /// @todo process incoming high-priority tasks?
      hwthread_pollpause();
      int state = done.fetch_add(0, std::memory_order_acquire);
      if (state == HANDOVER) return process(done);
      if (state == DONE) return;
    }
  }

} // namespace async
} // namespace mythos
