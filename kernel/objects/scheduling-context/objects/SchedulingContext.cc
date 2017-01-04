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

#include "cpu/hwthreadid.hh"
#include "objects/SchedulingContext.hh"
#include "objects/ISchedulable.hh"
#include "objects/CapEntry.hh"
#include "objects/UntypedMemory.hh"
#include "objects/mlog.hh"
#include "boot/memory-root.hh"

namespace mythos {

  void SchedulingContext::unbind(handle_t* ec)
  {
    MLOG_DETAIL(mlog::sched, "unbind", ec);
    readyQueue.remove(ec);
    preempt(ec); /// @todo race when ec is deleted too early
  }

  void SchedulingContext::ready(handle_t* ec)
  {
    MLOG_INFO(mlog::sched, "ready", ec);
    ASSERT(ec != nullptr);

    readyQueue.remove(ec); /// @todo do not need to remove if already on the queue, just do nothing then

    // set current_ec or push to the ready queue
    handle_t* current = current_ec;
    handle_t* const removed = reinterpret_cast<handle_t*>(REMOVED);
    while (true) {
      if (current == nullptr) {
        if (current_ec.compare_exchange_strong(current, ec)) return preempt(); // wake him up
      } else if (current == removed) {
        if (current_ec.compare_exchange_strong(current, ec)) return;
      } else if (!current->get()->isReady()) {
        if (current_ec.compare_exchange_strong(current, ec)) return preempt(); // no longer ready
      } else {
        readyQueue.push(ec);
        return;
      }
    }
  }

  void SchedulingContext::preempt(Tasklet* t, IResult<void>* res, handle_t* ec)
  {
    MLOG_DETAIL(mlog::sched, "preempt", DVAR(t), DVAR(ec));
    ASSERT(ec != nullptr);
    handle_t* const removed = reinterpret_cast<handle_t*>(REMOVED);
    if (!current_ec.compare_exchange_strong(ec, removed))
      return res->response(t, Error::SUCCESS); // was not selected, nothing to do
    // the ec was selected, make sure it is not running
    if (&getLocalPlace() == home)
      return res->response(t, Error::SUCCESS); // we are on the home thread already, nothing to do
    if (preempting.test_and_set()) return; // we are preempting the home already, nothing to do
    MLOG_DETAIL(mlog::sched, "send preemption message", DVAR(t), DVAR(home));
    home->run(tasklet.set([=](Tasklet* t){
          res->response(t, Error::SUCCESS);
          preempting.clear();
        }));
  }

  void SchedulingContext::preempt(handle_t* ec)
  {
    MLOG_DETAIL(mlog::sched, "preempt", DVAR(ec));
    ASSERT(ec != nullptr);
    handle_t* const removed = reinterpret_cast<handle_t*>(REMOVED);
    if (!current_ec.compare_exchange_strong(ec, removed)) return; // was not selected, nothing to do
    preempt(); // the thread was selected, make sure it is not running
  }

  void SchedulingContext::preempt() {
    MLOG_DETAIL(mlog::sched, "preempt", DVAR(home));
    if (&getLocalPlace() == home) return; // we are on the home thread already, nothing to do
    if (preempting.test_and_set()) return; // we are preempting the home already, nothing to do
    home->run(tasklet.set([=](Tasklet*){ preempting.clear(); }));
  }

  void SchedulingContext::yield(handle_t* ec)
  {
    MLOG_DETAIL(mlog::sched, "yield", DVAR(ec));
    ASSERT(ec != nullptr);
    if (current_ec != ec) return; // was not selected, nothing to do
    if (readyQueue.empty()) return; // no other waiting thread, nothing to do
    handle_t* const removed = reinterpret_cast<handle_t*>(REMOVED);
    if (current_ec.compare_exchange_strong(ec, removed)) {
      preempt(); // the thread was selected, make sure it is not running
      if (ec->get()->isReady()) readyQueue.push(ec); // append to the ready list
    }
  }

  void SchedulingContext::tryRunUser()
  {
    MLOG_DETAIL(mlog::sched, "tryRunUser");
    ASSERT(&getLocalPlace() == home);
    handle_t* current = current_ec;
    handle_t* const removed = reinterpret_cast<handle_t*>(REMOVED);
    while (true) {
      if (current != nullptr && current != removed && current->get()->isReady()) {
        current->get()->resume(); // will not return if successful, otherwise no longer ready
        // replace and retry
      } else {
        handle_t* next = readyQueue.pull();
        while (next != nullptr && !next->get()->isReady()) next = readyQueue.pull();
        if (current == nullptr && next == nullptr) {
          // go sleeping because we don't have anything to run
          return;
        }
        // next line is the only one writing nullptr to current_ec
        if (current_ec.compare_exchange_strong(current, next)) continue; // retry with next
        if (next != nullptr) readyQueue.push(next); // stuff next back to the queue
        // retry with the new current
      }
    }
  }

} // namespace mythos
