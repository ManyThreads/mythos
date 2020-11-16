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
 * Copyright 2020 Philipp Gypser and contributors, BTU Cottbus-Senftenberg
 */
#pragma once

#include "async/NestedMonitorDelegating.hh"
#include "objects/IFactory.hh"
#include "objects/IKernelObject.hh"
#include "cpu/hwthreadid.hh"
#include "mythos/protocol/ProcessorManagement.hh"
#include "boot/load_init.hh"
#include "boot/mlog.hh"

namespace mythos {

class ProcessorManagement
  : public IKernelObject
{
public:
  optional<void const*> vcast(TypeId) const override { THROW(Error::TYPE_MISMATCH); }
  optional<void> deleteCap(CapEntry&, Cap, IDeleter&) override { RETURN(Error::SUCCESS); }
  void deleteObject(Tasklet*, IResult<void>*) override {}
  void invoke(Tasklet* t, Cap self, IInvocation* msg) override;

  void init(){
      MLOG_ERROR(mlog::boot, "PM::init");
    numFree = 0;
      for (cpu::ThreadID id = 0; id < cpu::getNumThreads(); ++id) {
      MLOG_ERROR(mlog::boot, "SC ", id, &boot::getScheduler(id), image2kernel(&boot::getScheduler(id)), this);
        sc[id].initRoot(Cap(image2kernel(&boot::getScheduler(id))));
        free[numFree] = id;
        numFree++;
      }
  }

public:
  ProcessorManagement()
  {
    sc = image2kernel(&mySC[0]);
    numFree = 0;
  }

  Error invoke_allocCore(Tasklet*, Cap, IInvocation* msg);

  optional<cpu::ThreadID> alloc(){
      optional<cpu::ThreadID> ret;
      if(numFree > 0){
        numFree--;
        ret = free[numFree];
        MLOG_ERROR(mlog::boot, "alloc", free[numFree]);
      }
      return ret;
  }

  Error invoke_freeCore(Tasklet*, Cap, IInvocation* msg);
protected:
  async::NestedMonitorDelegating monitor;

private:
  CapEntry mySC[MYTHOS_MAX_THREADS];
  CapEntry *sc;
  cpu::ThreadID free[MYTHOS_MAX_THREADS];
  unsigned numFree;

};

  Error ProcessorManagement::invoke_allocCore(Tasklet*, Cap, IInvocation* msg){
      MLOG_ERROR(mlog::boot, "PM::invoke_allocCore");
      if(numFree > 0){
        numFree--;
        auto id = free[numFree];
        MLOG_ERROR(mlog::boot, "alloc core id ", id);

        auto dstEntry = msg->lookupEntry(init::SCHEDULERS_START+id, 32, true);
        if (!dstEntry) return dstEntry.state();
        MLOG_ERROR(mlog::boot, "bla");

        //cap::inherit(sc[id], sc[id].cap(), **dstEntry, (*dstEntry)->cap().asReference());
        cap::reference(sc[id], **dstEntry, sc[id].cap());
        
        MLOG_ERROR(mlog::boot, "blub");
        msg->getMessage()->write<protocol::ProcessorManagement::RetAllocCore>(init::SCHEDULERS_START+id);
      }else{
      msg->getMessage()->write<protocol::ProcessorManagement::RetAllocCore>(null_cap);
      }
      return Error::SUCCESS;
  }

  Error ProcessorManagement::invoke_freeCore(Tasklet*, Cap, IInvocation* msg){
    auto data = msg->getMessage()->cast<protocol::ProcessorManagement::FreeCore>();
    return Error::SUCCESS;
  }
} // namespace mythos
