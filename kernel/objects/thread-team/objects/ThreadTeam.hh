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

      bool tryRunEC(ExecutionContext* ec);
      Error invokeTryRunEC(Tasklet* t, Cap, IInvocation* msg);
      Error invokeDemandRunEC(Tasklet* t, Cap, IInvocation* msg);
      Error invokeForceRunEC(Tasklet* t, Cap, IInvocation* msg);
      Error invokeRunNextToEC(Tasklet* t, Cap, IInvocation* msg);

      void bind(optional<ProcessorAllocator*> ) {
        MLOG_DETAIL(mlog::pm, "bind processor allocator");
      }

      void unbind(optional<ProcessorAllocator*> ) {
        MLOG_ERROR(mlog::pm, "ERROR: unbind processor allocator");
      }

      void notifyIdle(Tasklet* t, cpu::ThreadID id) override {
        MLOG_DETAIL(mlog::pm, __func__, DVAR(id));
        monitor.request(t, [=](Tasklet* t){
            removeUsed(id);
            pushFree(id);
            monitor.responseAndRequestDone();
        }); 
      }
    private:
      void pushFree(cpu::ThreadID id){
        MLOG_DETAIL(mlog::pm, __func__, DVAR(id));
        freeList[nFree] = id;
        nFree++;
      }

      optional<cpu::ThreadID> popFree(){
        MLOG_DETAIL(mlog::pm, __func__);
        optional<cpu::ThreadID> ret;
        if(nFree > 0){
          nFree--;
          ret = freeList[nFree];
        }
        return ret;
      }

      void pushUsed(cpu::ThreadID id){
        MLOG_DETAIL(mlog::pm, __func__, DVAR(id));
        usedList[nUsed] = id;
        nUsed++;
      }

      void removeUsed(cpu::ThreadID id){
        MLOG_DETAIL(mlog::pm, __func__, DVAR(id));
        for(unsigned i = 0; i < nUsed; i++){
          if(usedList[i] == id){
            nUsed--;
            for(; i < nUsed; i++){
              usedList[i] = usedList[i+1];
            }
            return;
          }
        }
        MLOG_ERROR(mlog::pm, "ERROR: did not find used ThreadID ", id);
      }


    private:
      IAsyncFree* memory;
      async::NestedMonitorDelegating monitor;

      friend class ThreadTeamFactory;
      CapRef<ThreadTeam,ProcessorAllocator> pa;
      cpu::ThreadID freeList[MYTHOS_MAX_THREADS];
      unsigned nFree;
      cpu::ThreadID usedList[MYTHOS_MAX_THREADS];
      unsigned nUsed;

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
