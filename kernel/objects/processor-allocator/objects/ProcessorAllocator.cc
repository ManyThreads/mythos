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


#include "objects/ProcessorAllocator.hh"
#include "objects/mlog.hh"

namespace mythos {

/* IKernelObject */ 
  optional<void> ProcessorAllocator::deleteCap(CapEntry&, Cap, IDeleter&)
  {
    MLOG_DETAIL(mlog::pm, __func__);
    RETURN(Error::SUCCESS);
  }

  void ProcessorAllocator::deleteObject(Tasklet*, IResult<void>*)
  {
    MLOG_DETAIL(mlog::pm, __func__);
  }

  void ProcessorAllocator::invoke(Tasklet* t, Cap self, IInvocation* msg)
  {
    MLOG_DETAIL(mlog::pm, __func__, DVAR(t), DVAR(msg));
    monitor.request(t, [=](Tasklet* t){
        Error err = Error::NOT_IMPLEMENTED;
        switch (msg->getProtocol()) {
        case protocol::ProcessorAllocator::proto:
          err = protocol::ProcessorAllocator::dispatchRequest(this, msg->getMethod(), t, self, msg);
          break;
        }
        if (err != Error::INHIBIT) {
          msg->replyResponse(err);
          monitor.requestDone();
        }
      } );
  }

/* IResult<void> */
  void ProcessorAllocator::response(Tasklet* /*t*/, optional<void> res){
    MLOG_DETAIL(mlog::pm, "revoke response:", res.state(), DVAR(toBeFreed));
    free(toBeFreed);
    toBeFreed = 0;
  }

/* ProcessorAllocator */
  ProcessorAllocator::ProcessorAllocator()
      : sc(image2kernel(&mySC[0]))
    {}

  void ProcessorAllocator::init(){
    MLOG_DETAIL(mlog::pm, "PM::init");
    for (cpu::ThreadID id = 0; id < cpu::getNumThreads(); ++id) {
      sc[id].initRoot(Cap(image2kernel(&boot::getScheduler(id))));
      free(id);
    }
  }

  Error ProcessorAllocator::invokeAlloc(Tasklet*, Cap, IInvocation* msg){
    MLOG_DETAIL(mlog::pm, __func__);
    auto id = alloc();

    if(id){
      MLOG_DETAIL(mlog::pm, "allocated ", DVAR(*id));

      auto data = msg->getMessage()->read<protocol::ProcessorAllocator::Alloc>();

      optional<CapEntry*> dstEntry;
      if(data.dstSpace() == null_cap){ // direct access
        dstEntry = msg->lookupEntry(init::SCHEDULERS_START+*id, 32, true); // lookup for write access
        if (!dstEntry){ 
          MLOG_WARN(mlog::pm, "Warning: cannot find dstEntry!");
          free(*id);
          return dstEntry.state();
        }
      
      }else{ // indirect access
        TypedCap<ICapMap> dstSpace(msg->lookupEntry(data.dstSpace()));
        if (!dstSpace){ 
          MLOG_WARN(mlog::pm, "Warning: cannot find dstSpace!");
          free(*id);
          return dstSpace.state();
        }

        //auto dstEntryRef = dstSpace.lookup(data.dstPtr, data.dstDepth, true); // lookup for write
        auto dstEntryRef = dstSpace.lookup(init::SCHEDULERS_START+*id, 32, true); // lookup for write
        if (!dstEntryRef){ 
          MLOG_WARN(mlog::pm, "Warning: cannot find dstEntryRef!");
          free(*id);
          return dstEntryRef.state();
        }

        dstEntry = dstEntryRef->entry;
      }

      auto res = cap::reference(sc[*id], **dstEntry, sc[*id].cap());
      if(res){
        msg->getMessage()->write<protocol::ProcessorAllocator::RetAlloc>(init::SCHEDULERS_START+*id);
        MLOG_DETAIL(mlog::pm, "map new sc ", DVAR(*id));
      }else{
        MLOG_WARN(mlog::pm, "Warning: cannot create SC entry!");
        free(*id);
        msg->getMessage()->write<protocol::ProcessorAllocator::RetAlloc>(null_cap);
      }

    }else{
      MLOG_WARN(mlog::pm, "allocation failed: no free cores available!");
      msg->getMessage()->write<protocol::ProcessorAllocator::RetAlloc>(null_cap);
    }
    return Error::SUCCESS;
  }

  // todo: implement new revokation mechanism that suits this scenario
  Error ProcessorAllocator::invokeFree(Tasklet* /*t*/, Cap, IInvocation* /*msg*/){
    MLOG_ERROR(mlog::pm, __func__, " NYI!");  
    //auto data = msg->getMessage()->cast<protocol::ProcessorAllocator::Free>();
    //ASSERT(data->sc() >= init::SCHEDULERS_START);
    //cpu::ThreadID id = data->sc() - init::SCHEDULERS_START;
    //ASSERT(id < cpu::getNumThreads());
    //MLOG_ERROR(mlog::pm, "free SC", DVAR(data->sc()), DVAR(id));
    //toBeFreed = id;
    //revokeOp._revoke(t, this, sc[id], this);
    return Error::SUCCESS;
  }

  void ProcessorAllocator::freeSC(Tasklet* t, cpu::ThreadID id){
    MLOG_DETAIL(mlog::pm, "freeSC", DVAR(id));
    monitor.request(t, [=](Tasklet* t){
      MLOG_DETAIL(mlog::pm, "monitor free", DVAR(id));
      toBeFreed = id;
      revokeOp._revoke(t, this, sc[id], this);
    }
    );
  }

/* LiFoProcessorAllocator */
  LiFoProcessorAllocator::LiFoProcessorAllocator()
      : nFree(0)
    {}

  optional<cpu::ThreadID> LiFoProcessorAllocator::alloc(){
    optional<cpu::ThreadID> ret;
    if(nFree > 0){
      nFree--;
      ret = freeList[nFree];
    }
    return ret;
  }

  void LiFoProcessorAllocator::free(cpu::ThreadID id) {
      freeList[nFree] = id;
      nFree++;
  }
} // namespace mythos
