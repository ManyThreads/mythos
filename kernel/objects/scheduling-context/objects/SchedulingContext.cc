/* -*- mode:C++; indent-tabs-mode:nil; -*- */
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

#include "cpu/hwthreadid.hh"
#include "objects/SchedulingContext.hh"
#include "objects/ISchedulable.hh"
#include "objects/CapEntry.hh"
#include "objects/KernelMemory.hh"
#include "objects/mlog.hh"
#include "boot/memory-root.hh"

namespace mythos {

    void SchedulingContext::bind(handle_t*) 
    {
    }
    
    void SchedulingContext::unbind(handle_t* ec)
    {
        ASSERT(ec != nullptr);
        MLOG_DETAIL(mlog::sched, "unbind", DVAR(ec->get()));
        readyQueue.remove(ec);
        current_handle.store(nullptr);
    }

    void SchedulingContext::ready(handle_t* ec)
    {
        ASSERT(ec != nullptr);
        MLOG_INFO(mlog::sched, "ready", DVAR(ec->get()));

        // do nothing if it is the current execution context
        auto current = current_handle.load();

	// ATTENTION: RACE CONDITION
        //if (current == ec) return; /// @todo can this actually happen? 
        
        // add to the ready queue
        readyQueue.remove(ec); /// @todo do not need to remove if already on the queue, just do nothing then. This needs additional information in handle_t of LinkedList
        readyQueue.push(ec);

        // wake up the hardware thread if it has no execution context running
	// or if if current ec got ready in case of race condition
        if (current == nullptr || current == ec) home->preempt();
    }

    void SchedulingContext::tryRunUser()
    {
        MLOG_DETAIL(mlog::sched, "tryRunUser");
        ASSERT(&getLocalPlace() == home);
        while (true) {
            auto current = current_handle.load();
            if (current != nullptr) {
                MLOG_DETAIL(mlog::sched, "try current", current, DVAR(current->get()));
                auto loaded = current_ec->load();
                if (loaded != current->get()) {
                    if (loaded != nullptr) loaded->saveState();
                    current->get()->loadState();
                }
                current->get()->resume(); // if it returns, the ec is blocked
                current_handle.store(nullptr);
            }
            MLOG_DETAIL(mlog::sched, "try from ready list");
            // something on the ready list?
            auto next = readyQueue.pull();
            while (next != nullptr && !next->get()->isReady()) next = readyQueue.pull();
            if (next == nullptr) {
                // go sleeping because we don't have anything to run
                MLOG_DETAIL(mlog::sched, "empty ready list, going to sleep");
                return;
            }
            current_handle.store(next);
            // now retry
        }
    }

} // namespace mythos
