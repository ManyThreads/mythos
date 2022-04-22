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
#include "cpu/mwait.hh"
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
        MLOG_INFO(mlog::sched, "unbind", DVAR(ec->get()));
        readyQueue.remove(ec);
        priorityQueue.remove(ec);
        current_handle.store(nullptr);
        if(readyQueue.empty()){
          MLOG_INFO(mlog::sched, "call idleSC");
          auto ni = myNI.load();
          if(ni != nullptr){
            ni->notifyIdle();
          }else{
            MLOG_WARN(mlog::sched, "No IdleNotify registered!");
          }
        }else{
          MLOG_INFO(mlog::sched, "ready queue not empty!");
        }   
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
        priorityQueue.remove(ec); /// @todo do not need to remove if already on the queue, just do nothing then. This needs additional information in handle_t of LinkedList
        if(ec->get()->hasPriority()){
          priorityQueue.push(ec);
          home->preempt(); // trigger reschedule to run priority ec 
        }else{
          readyQueue.push(ec);

          // wake up the hardware thread if it has no execution context running
    // or if if current ec got ready in case of race condition
          //if (current == nullptr || current == ec) home->preempt();
          if (current == nullptr || current == ec){
            //MLOG_ERROR(mlog::sched, "wake SC");
            //home->preempt();
            wake();
          } 
        }
    }

    void SchedulingContext::tryRunUser()
    {
        MLOG_DETAIL(mlog::sched, "tryRunUser");
        ASSERT(&getLocalPlace() == home);
        while (true) {
            auto current = current_handle.load();
            if (current != nullptr) {
                if(current->get()->hasPriority() || priorityQueue.empty()){
                    MLOG_DETAIL(mlog::sched, "try current", current, DVAR(current->get()));
                    auto loaded = current_ec->load();
                    if (loaded != current->get()) {
                        if (loaded != nullptr) loaded->saveState();
                        current->get()->loadState();
                    }
                    current->get()->resume(); // if it returns, the ec is blocked
                    current_handle.store(nullptr);
                }else{
                    MLOG_DETAIL(mlog::sched, "try switching to priority EC from non-prio EC ", current, DVAR(current->get()));
                    readyQueue.push(current);
                    current_handle.store(nullptr);
                }
            }

            MLOG_DETAIL(mlog::sched, "try from priority list");
            sleepFlag.store(true);
            auto next = priorityQueue.pull();
            while (next != nullptr && !next->get()->isReady()) next = priorityQueue.pull();
            if (next == nullptr) {
                MLOG_DETAIL(mlog::sched, "empty priority list, try ready list");
                // something on the ready list?
                next = readyQueue.pull();
                while (next != nullptr && !next->get()->isReady()) next = readyQueue.pull();
                if (next == nullptr) {
                    // go sleeping because we don't have anything to run
                    MLOG_DETAIL(mlog::sched, "empty ready list, going to sleep");
                    return;
                }
            }else{
                MLOG_DETAIL(mlog::sched, "found priority EC", next, DVAR(next->get()));
            } 
            current_handle.store(next);
            // now retry
        }
    }

    void SchedulingContext::sleep(){
        //MLOG_ERROR(mlog::sched, "empty ready list, going to sleep");
        SleepMode sm = HALT;
        //SleepMode sm = SPINNING;
        auto ni = myNI.load();
        if(ni != nullptr){
          sm = ni->getSleepMode();
        }else{
          //MLOG_ERROR(mlog::sched, "No IdleNotify registered!");
        }

        unsigned long hint = 0;
        switch(sm){
          case SPINNING:
            //MLOG_ERROR(mlog::sched, "Spinning");
            hint = cpu::mwaitHint(0,0); 
            break;
          case HALT:
            //MLOG_ERROR(mlog::sched, "Halt");
            hint = cpu::mwaitHint(1,0); 
            break;
          case ENHANCEDHALT:
            //MLOG_ERROR(mlog::sched, "EHalt");
            hint = cpu::mwaitHint(1,1); 
            break;
          case DEEPSLEEP:
            //MLOG_ERROR(mlog::sched, "Deep sleep");
            hint = cpu::mwaitHint(3,1); 
            break;
          default:
            MLOG_ERROR(mlog::sched, "Unknown sleep mode ", DVAR(sm));
        }
      

        cpu::clflush(&sleepFlag);
        cpu::barrier();
        //cpu::clearInt();
        //cpu::barrier();
        cpu::monitor(&sleepFlag, 0, 0);
        cpu::barrier();
        if(sleepFlag.load()){
          cpu::mwait(hint,1);
        }else{
          //MLOG_ERROR(mlog::sched, "mwait failed");
        }
        //MLOG_ERROR(mlog::sched, "woken up");
    }

    void SchedulingContext::wake(){ 
      sleepFlag.store(false); 
      cpu::clflush(&sleepFlag);
      cpu::barrier();
    }
} // namespace mythos
