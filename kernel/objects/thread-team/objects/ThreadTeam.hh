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
 * Copyright 2021 Philipp Gypser, BTU Cottbus-Senftenberg
 */
#pragma once

#include "async/NestedMonitorDelegating.hh"
#include "async/IResult.hh"
#include "objects/IFactory.hh"
#include "objects/IKernelObject.hh"
#include "cpu/hwthreadid.hh"
#include "mythos/protocol/ThreadTeam.hh"
#include "boot/mlog.hh"
#include "objects/RevokeOperation.hh"
#include "objects/ProcessorAllocator.hh"
#include "objects/ExecutionContext.hh"
#include "objects/SchedulingContext.hh"

namespace mythos {

  class ThreadTeam
    : public IKernelObject
    , public INotifyIdle
    , public IResult<cpu::ThreadID>
    , public IResult<void>
  {
    public:
      ThreadTeam(IAsyncFree* memory);

    /* IKernelObject */
      optional<void> deleteCap(CapEntry&, Cap self, IDeleter& del) override;
      void deleteObject(Tasklet* t, IResult<void>* r) override;
      void invoke(Tasklet* t, Cap self, IInvocation* msg) override;
      optional<void const*> vcast(TypeId id) const override {
        if (id == typeId<ThreadTeam>()) return this;
        THROW(Error::TYPE_MISMATCH);
      }

    /* IResult */
      //called from ProcessorAllocator::alloc
      void response(Tasklet* t, optional<cpu::ThreadID> id);
      //called from ExecutionContext::setSchedulingContext
      void response(Tasklet* t, optional<void> bound);
  
      // only for init EC
      bool tryRun(ExecutionContext* ec);

      // invocations
      Error invokeTryRunEC(Tasklet* t, Cap, IInvocation* msg);
      Error invokeRevokeDemand(Tasklet* t, Cap, IInvocation* msg);
      Error invokeRunNextToEC(Tasklet* t, Cap, IInvocation* msg);
      Error invokeSetLimit(Tasklet* t, Cap, IInvocation* msg);

      void bind(optional<ProcessorAllocator*> paPtr);
      void unbind(optional<ProcessorAllocator*> );

      void bind(optional<ExecutionContext*> /*ec*/);
      void unbind(optional<ExecutionContext*> ec);

      void notifyIdle(Tasklet* t, cpu::ThreadID id) override;

    private:
      void tryRunAt(Tasklet* t, ExecutionContext* ec, cpu::ThreadID id);

      void pushFree(cpu::ThreadID id);
      optional<cpu::ThreadID> popFree();

      void pushUsed(cpu::ThreadID id);
      void removeUsed(cpu::ThreadID id);
      optional<cpu::ThreadID> popUsed();
      
      bool enqueueDemand(CapEntry* ec);
      bool removeDemand(ExecutionContext* ec, bool resetRef);
      bool tryRunDemandAt(Tasklet* t, cpu::ThreadID id);
      void dumpDemand();
      bool limitReached() { return limit && numAllocated >= limit;  } 

    private:
      LinkedList<IKernelObject*>::Queueable del_handle = {this};
      IAsyncFree* memory;
      async::NestedMonitorDelegating monitor;

    /* state for async operation handling */
      IInvocation* tmp_msg;
      static constexpr cpu::ThreadID INV_ID = cpu::ThreadID(-1);
      cpu::ThreadID tmp_id;
      ExecutionContext* tmp_ec;
      ExecutionContext* rm_ec;
      enum OperationalState{
        IDLE,
        INVOCATION,
        SC_NOTIFY
      };
      OperationalState state;

      friend class ThreadTeamFactory;
      CapRef<ThreadTeam,ProcessorAllocator> paRef;
      ProcessorAllocator* pa;
      Tasklet paTasklet;
      cpu::ThreadID freeList[MYTHOS_MAX_THREADS];
      unsigned nFree;
      cpu::ThreadID usedList[MYTHOS_MAX_THREADS];
      unsigned nUsed;
      CapRef<ThreadTeam, ExecutionContext> demandEC[MYTHOS_MAX_THREADS];
      // index < nDemand = demandEC slot in use
      // index >= nDemand = demandEC slot is free
      unsigned demandList[MYTHOS_MAX_THREADS]; // indexes to demandEC
      unsigned nDemand; 
      unsigned limit;
      unsigned numAllocated;
  };

  class ThreadTeamFactory : public FactoryBase
  {
  public:
    typedef protocol::ThreadTeam::Create message_type;

    static optional<ThreadTeam*>
    factory(CapEntry* dstEntry, CapEntry* memEntry, Cap memCap, IAllocator* mem, CapEntry* pae);

    Error factory(CapEntry* dstEntry, CapEntry* memEntry, Cap memCap,
                  IAllocator* mem, IInvocation* msg) const override {
      MLOG_DETAIL(mlog::pm, __PRETTY_FUNCTION__);
      auto data = msg->getMessage()->read<message_type>();
      auto paEntry = msg->lookupEntry(data.pa());
      if (!paEntry){
        MLOG_ERROR(mlog::pm, "ERROR: cannot find processor allocator entry reference!");
        return Error::INVALID_CAPABILITY; 
      }
      return factory(dstEntry, memEntry, memCap, mem, *paEntry).state();
    }
  };

} // namespace mythos
