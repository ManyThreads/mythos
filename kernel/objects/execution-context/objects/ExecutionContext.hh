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

#include <cstdint>
#include <atomic>

#include "cpu/kernel_entry.hh"
#include "cpu/fpu.hh"
#include "async/NestedMonitorDelegating.hh"
#include "objects/IKernelObject.hh"
#include "objects/ISchedulable.hh"
#include "objects/IScheduler.hh"
#include "objects/IFactory.hh"
#include "objects/IPageMap.hh"
#include "objects/INotifiable.hh"
#include "objects/ISignalable.hh"
#include "objects/IPortal.hh"
#include "objects/CapEntry.hh"
#include "objects/CapRef.hh"
#include "mythos/protocol/KernelObject.hh"
#include "mythos/protocol/ExecutionContext.hh"

namespace mythos {

  class ExecutionContext final
    : public IKernelObject
    , public ISchedulable
    , public IPortalUser
    , public ISignalable
  {
  public:

    typedef uint16_t flag_t;
    enum Flags : flag_t
    {
      IS_WAITING = 1<<0, // used by wait() syscall
      IS_TRAPPED = 1<<1, // used by suspend/resume invocations and trap/exception handler
      NO_AS      = 1<<2, // set if address space is missing
      NO_SCHED   = 1<<3, // set if scheduler is missing
      IN_WAIT    = 1<<5, // EC is in wait() syscall, next sysret should return a KEvent
      IS_NOTIFIED     = 1<<6, // used by notify() syscall for binary semaphore
      REGISTER_ACCESS = 1<<7, // accessing registers
      NOT_LOADED   = 1<<8, // CPU state is not loaded
      DONT_PREEMPT  = 1<<9, // somebody else will send the preemption
      NOT_RUNNING  = 1<<10, // EC is not running
      BLOCK_MASK = IS_WAITING | IS_TRAPPED | NO_AS | NO_SCHED | REGISTER_ACCESS
    };

    ExecutionContext(IAsyncFree* memory);
    virtual ~ExecutionContext() {}

    optional<void> setAddressSpace(optional<CapEntry*> pagemapref);
    void unsetAddressSpace() { _as.reset(); }

    /// only for initial setup
    optional<void> setSchedulingContext(optional<CapEntry*> sce);
    optional<void> setSchedulingContext(Tasklet* t, IInvocation* msg, optional<CapEntry*> sce);
    Error unsetSchedulingContext();

    optional<void> setCapSpace(optional<CapEntry*> capmapref);
    void unsetCapSpace() { _cs.reset(); }

    void setEntryPoint(uintptr_t rip);
    void setTrapped(bool val);

    cpu::ThreadState& getThreadState() { return threadState; }

    optional<void> setRegisters(const mythos::protocol::ExecutionContext::Amd64Registers&);
    optional<void> setBaseRegisters(uint64_t fs_base, uint64_t gs_base);

  public: // INotifiable interface
    void notify(INotifiable::handle_t* event) override;
    void denotify(INotifiable::handle_t* event) override;

  public: // ISchedulable interface
    bool isReady() const override { return !isBlocked(flags.load()); }
    void resume() override;
    void handleTrap() override;
    void handleInterrupt() override { setFlags(NOT_RUNNING); }
    void handleSyscall() override;
    optional<void> syscallInvoke(CapPtr portal, CapPtr dest, uint64_t user);
    void loadState() override;
    void saveState() override;

  public: // ISignalable interface
    optional<void> signal(CapData data) override;

  public: // IPortalUser interface
    optional<CapEntryRef> lookupRef(CapPtr ptr, CapPtrDepth ptrDepth, bool writeable) override;

  public: // IKernelObject interface
    optional<void const*> vcast(TypeId id) const override {
      if (id == typeId<ISchedulable>()) return static_cast<ISchedulable const*>(this);
      if (id == typeId<IPortalUser>()) return static_cast<IPortalUser const*>(this);
      if (id == typeId<ISignalable>()) return static_cast<ISignalable const*>(this);
      THROW(Error::TYPE_MISMATCH);
    }
    optional<void> deleteCap(CapEntry&, Cap self, IDeleter& del) override;
    void deleteObject(Tasklet* t, IResult<void>* r) override;
    void invoke(Tasklet* t, Cap self, IInvocation* msg) override;

  protected:
    friend struct protocol::ExecutionContext;
    friend struct protocol::KernelObject;

    Error invokeConfigure(Tasklet* t, Cap, IInvocation* msg);
    Error invokeReadRegisters(Tasklet* t, Cap self, IInvocation* msg);
    void  readThreadRegisters(Tasklet* t, optional<void>);
    Error invokeWriteRegisters(Tasklet* t, Cap self, IInvocation* msg);
    void writeThreadRegisters(Tasklet* t, optional<void>);
    Error invokeResume(Tasklet* t, Cap self, IInvocation* msg);
    Error invokeSuspend(Tasklet* t, Cap self, IInvocation* msg);
    void suspendThread(Tasklet* t, optional<void>);
    Error getDebugInfo(Cap self, IInvocation* msg);
    Error invokeSetFSGS(Tasklet* t, Cap self, IInvocation* msg);

  protected:
    friend class CapRefBind;
    void bind(optional<IPageMap*>);
    void bind(optional<ICapMap*>) {}
    void bind(optional<IScheduler*>);
    void unbind(optional<IPageMap*>);
    void unbind(optional<ICapMap*>) {}
    void unbind(optional<IScheduler*>);

    flag_t setFlags(flag_t f) { return flags.fetch_or(f); }
    flag_t clearFlags(flag_t f) { return flags.fetch_and(flag_t(~f)); }
    void setFlagsSuspend(flag_t f);
    void clearFlagsResume(flag_t f);
    bool isBlocked(flag_t f) const { return (f & BLOCK_MASK) != 0; }
    bool needPreemption(flag_t f) const { return (f & DONT_PREEMPT) == 0; }

  private:
    async::NestedMonitorDelegating monitor;
    INotifiable::list_t notificationQueue;
    std::atomic<flag_t> flags;
    CapRef<ExecutionContext,IPageMap> _as;
    CapRef<ExecutionContext,ICapMap> _cs;
    CapRef<ExecutionContext,IScheduler> _sched;
    /// @todo reference/link to exception handler (portal/endpoint?)

    // the hardware thread where the fpu state is currently loaded
    std::atomic<async::Place*> currentPlace = {nullptr};
    IScheduler::handle_t ec_handle = {this};

    IInvocation* msg;
    async::MSink<ExecutionContext, void, &ExecutionContext::readThreadRegisters>
      readSleepResponse = {this};
    async::MSink<ExecutionContext, void, &ExecutionContext::writeThreadRegisters>
      writeSleepResponse = {this};
    async::MSink<ExecutionContext, void, &ExecutionContext::suspendThread>
      sleepResponse = {this};

    cpu::ThreadState threadState;
    cpu::FpuState fpuState;

    LinkedList<IKernelObject*>::Queueable del_handle = {this};
    IAsyncFree* memory;

    Tasklet exitTask; 
  };

  class ExecutionContextFactory : public FactoryBase
  {
  public:
    typedef protocol::ExecutionContext::Create message_type;

    static optional<ExecutionContext*>
    factory(CapEntry* dstEntry, CapEntry* memEntry, Cap memCap, IAllocator* mem, message_type* data = nullptr, IInvocation* msg = nullptr);

    Error factory(CapEntry* dstEntry, CapEntry* memEntry, Cap memCap,
                  IAllocator* mem, IInvocation* msg) const override {
      auto data = msg->getMessage()->read<message_type>();
      return factory(dstEntry, memEntry, memCap, mem, &data, msg).state();
    }
  };

} // namespace mythos
