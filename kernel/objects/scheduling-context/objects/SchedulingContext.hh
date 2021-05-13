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
#pragma once

#include "async/Place.hh"
#include "objects/IKernelObject.hh"
#include "objects/IScheduler.hh"
#include "async/SimpleMonitorHome.hh"
#include <cstdint>
#include "util/error-trace.hh"
#include "util/events.hh"

namespace mythos {

  class INotifyIdle{
    public:
      virtual void notifyIdle() = 0;
  };

  /** Scheduler for multiple application threads on a single hardware
   * thread. It implements an cooperative FIFO strategy that switches only
   * when the currently selected application thread becomes blocked.
   *
   * The scheduler has a queueable handle pointing to the currently
   * selected Execution Context and a FIFO queue of waiting ECs.
   *
   * If a new EC becomes ready and there is no selected EC, the new
   * one is selected and a wakeup message is sent to the hardware
   * thread. Otherwise, the newly ready EC is appended to the
   * queue. The implementation takes care of ECs that are enqueued
   * already.
   *
   * If an EC preempted and it is the selected EC, a preemptive message
   * is sent to the hardware thread in order to suspend the EC's
   * execution. If the EC is still ready (executable), it is appended
   * to the waiting queue if it is not empty or it stays the selected
   * EC if there is no other waiting. Preempting other ECs then the
   * selected one does nothing because their readiness is checked when
   * selecting them for execution later. Their ready state may change
   * multiple times without the need to update the queue.
   *
   * In order to yield execution to the next waiting EC, the selected
   * EC can preempt itself while staying ready.
   *
   * The hardware thread's main routine asks the scheduler to switch
   * to user mode. Then the currently selected EC is resumed if
   * present. This may fail because the EC is no longer ready. In that
   * case, the next EC is selected from the queue and resumed. This
   * repeats until one of the waiting ECs is actually ready or the
   * queue is empty. If no ready EC was found, there is no selected EC
   * and the control returns.
   */
  class SchedulingContext final
    : public IKernelObject
    , public IScheduler
  {
  public:
    SchedulingContext()
      : myNI(nullptr)
    { }
    void init(async::Place* home) { this->home = home; }
    virtual ~SchedulingContext() {}

    /** try to switch to the user mode.
     * Returns only if no ready execution context was available.
     */
    void tryRunUser();

  public: // IScheduler interface
    void bind(handle_t* ec_handle) override;
    void unbind(handle_t* ec_handle) override;
    void ready(handle_t* ec_handle) override;

    //todo: use capref
  public: //ThreadTeam
    void registerIdleNotify(INotifyIdle* ni){
      myNI.store(ni);
    };
    void resetIdleNotify(){
      myNI.store(nullptr);
    }

    async::Place* getHome(){ return home; }

  public: // IKernelObject interface
    optional<void> deleteCap(CapEntry&, Cap, IDeleter&) override { RETURN(Error::SUCCESS); }
    optional<void const*> vcast(TypeId id) const override {
      if (id == typeId<IScheduler>()) return static_cast<const IScheduler*>(this);
      if (id == typeId<SchedulingContext>()) return this;
      THROW(Error::TYPE_MISMATCH);
    }

  private:
    async::Place* home = nullptr;
    list_t readyQueue; //< the ready list of waiting execution contexts
    std::atomic<handle_t*> current_handle = {nullptr}; //< the currently selected execution context

    std::atomic<INotifyIdle*> myNI;
  };
} // namespace mythos
