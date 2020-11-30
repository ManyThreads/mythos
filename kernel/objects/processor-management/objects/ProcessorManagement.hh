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
#include "objects/RevokeOperation.hh"
#include "async/IResult.hh"

namespace mythos {

class ProcessorAllocator {
  public:
    virtual optional<cpu::ThreadID> alloc() = 0;
    virtual unsigned getNumFree() = 0;
    virtual void free(cpu::ThreadID id) = 0;
};

class LiFoProcessorAllocator : public ProcessorAllocator
{
  public:
    LiFoProcessorAllocator()
      : numFree(0)
    {}

    optional<cpu::ThreadID> alloc() override {
      optional<cpu::ThreadID> ret;
      if(numFree > 0){
        numFree--;
        ret = freeList[numFree];
      }
      return ret;
    }
    
    unsigned getNumFree() override { return numFree; }

    void free(cpu::ThreadID id) override {
        freeList[numFree] = id;
        numFree++;
    }
  private:
    unsigned numFree;
    cpu::ThreadID freeList[MYTHOS_MAX_THREADS];

};

class ProcessorManagement
  : public IKernelObject
  , public IResult<void>
{
public:
  optional<void const*> vcast(TypeId) const override { THROW(Error::TYPE_MISMATCH); }
  optional<void> deleteCap(CapEntry&, Cap, IDeleter&) override { RETURN(Error::SUCCESS); }
  void deleteObject(Tasklet*, IResult<void>*) override {}
  void invoke(Tasklet* t, Cap self, IInvocation* msg) override;

  virtual void response(Tasklet* t, optional<void> res) override{
    MLOG_INFO(mlog::pm, "revoke response:", res.state());
  }

  void init(){
      MLOG_INFO(mlog::pm, "PM::init");
      for (cpu::ThreadID id = 0; id < cpu::getNumThreads(); ++id) {
        sc[id].initRoot(Cap(image2kernel(&boot::getScheduler(id))));
        pa.free(id);
      }
  }

  ProcessorManagement()
    : sc(image2kernel(&mySC[0]))
  {}

  Error allocCore(Tasklet*, Cap, IInvocation* msg);

  Error freeCore(Tasklet*, Cap, IInvocation* msg);

  void freeCore(Tasklet* t, cpu::ThreadID id){
      MLOG_DETAIL(mlog::pm, "PM::freeCore", DVAR(id));
      monitor.request(t, [=](Tasklet* t){
        MLOG_DETAIL(mlog::pm, "monitor freeCore", DVAR(id));
        //revokeOp.revokeCap(t, this, sc[id], this);
        revokeOp._revoke(t, this, sc[id], this);
        pa.free(id);
      }
    );
  }
private:
  CapEntry *sc;
  CapEntry mySC[MYTHOS_MAX_THREADS];

protected:
  async::NestedMonitorDelegating monitor;
  RevokeOperation revokeOp = {monitor};
  friend class PluginProcessorManagement;
  LiFoProcessorAllocator pa;
};

  Error ProcessorManagement::allocCore(Tasklet*, Cap, IInvocation* msg){
    MLOG_DETAIL(mlog::pm, "PM::allocCore");
      auto id = pa.alloc();
      if(id){

        auto dstEntry = msg->lookupEntry(init::SCHEDULERS_START+*id, 32, true);
        if (!dstEntry) return dstEntry.state();

        //cap::inherit(sc[id], sc[id].cap(), **dstEntry, (*dstEntry)->cap().asReference());
        cap::reference(sc[*id], **dstEntry, sc[*id].cap());
        msg->getMessage()->write<protocol::ProcessorManagement::RetAllocCore>(init::SCHEDULERS_START+*id);
      }else{
      msg->getMessage()->write<protocol::ProcessorManagement::RetAllocCore>(null_cap);
      }
      return Error::SUCCESS;
  }

  Error ProcessorManagement::freeCore(Tasklet*, Cap, IInvocation* msg){
    MLOG_DETAIL(mlog::pm, "PM::freeCore");
    auto data = msg->getMessage()->cast<protocol::ProcessorManagement::FreeCore>();
    return Error::SUCCESS;
  }

} // namespace mythos
