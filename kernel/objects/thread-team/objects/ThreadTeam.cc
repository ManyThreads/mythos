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


#include "objects/ThreadTeam.hh"
#include "objects/mlog.hh"

namespace mythos {

/* IKernelObject */ 
  optional<void> ThreadTeam::deleteCap(CapEntry&, Cap, IDeleter&)
  {
    MLOG_DETAIL(mlog::pm, __func__);
    RETURN(Error::SUCCESS);
  }

  void ThreadTeam::deleteObject(Tasklet*, IResult<void>*)
  {
    MLOG_DETAIL(mlog::pm, __func__);
  }

  void ThreadTeam::invoke(Tasklet* t, Cap self, IInvocation* msg)
  {
    MLOG_DETAIL(mlog::pm, __func__, DVAR(t), DVAR(msg));
    monitor.request(t, [=](Tasklet* t){
        Error err = Error::NOT_IMPLEMENTED;
        switch (msg->getProtocol()) {
        case protocol::ThreadTeam::proto:
          err = protocol::ThreadTeam::dispatchRequest(this, msg->getMethod(), t, self, msg);
          break;
        }
        if (err != Error::INHIBIT) {
          msg->replyResponse(err);
          monitor.requestDone();
        }
      } );
  }

/* ThreadTeam */
  ThreadTeam::ThreadTeam(IAsyncFree* memory)
    : memory(memory)
    , nFree(0)
    , nUsed(0)
    , nDemand(0)
    {
      for(unsigned d = 0; d < MYTHOS_MAX_THREADS; d++){
        demandList[d] = d;
      }
    }

  bool ThreadTeam::tryRun(ExecutionContext* ec){
    MLOG_DETAIL(mlog::pm, __func__);
    auto pal = pa.get();
    ASSERT(pal);

    auto id = popFree();
    if(id){
      MLOG_DETAIL(mlog::pm, "take SC from Team ", DVAR(*id));
    }else{
      MLOG_DETAIL(mlog::pm, "try alloc SC from PA");
      id = pal->alloc();
    }
    if(id && tryRunAt(ec, *id)){
      pushUsed(*id);
      return true;
    }else{
      MLOG_ERROR(mlog::pm, "Cannot allocate SC for init EC");
    }
    return false;
  }

  bool ThreadTeam::tryRunAt(ExecutionContext* ec, cpu::ThreadID id){
    MLOG_DETAIL(mlog::pm, __func__);
    auto pal = pa.get();
    ASSERT(pal);

    auto sce = pal->getSC(id);
    TypedCap<SchedulingContext> sc(sce->cap());
    sc->registerThreadTeam(static_cast<INotifyIdle*>(this));
    auto ret = ec->setSchedulingContext(sce);
    if(ret){
      MLOG_DETAIL(mlog::pm, "Init EC bind SC", DVAR(id));
      return true;
    }else{
      MLOG_ERROR(mlog::pm, "ERROR: Init EC bind SC failed ", DVAR(id));
    }
    return false;
  }

  Error ThreadTeam::invokeTryRunEC(Tasklet* /*t*/, Cap, IInvocation* msg){
    MLOG_ERROR(mlog::pm, __func__);
    
    auto data = msg->getMessage()->read<protocol::ThreadTeam::TryRunEC>();
    auto ece = msg->lookupEntry(data.ec());
    auto ret = msg->getMessage()->cast<protocol::ThreadTeam::RetTryRunEC>();
    if(!ece){
      MLOG_ERROR(mlog::pm, "Error: Did not find EC!");
      return Error::INVALID_CAPABILITY;
    }

    TypedCap<ExecutionContext> ec(ece);

    if(ec && tryRun(*ec)){
      ret->setResponse(protocol::ThreadTeam::RetTryRunEC::ALLOCATED);
      return Error::SUCCESS;
    }

    if(data.allocType == protocol::ThreadTeam::DEMAND){
      if(enqueueDemand(*ece)){
        ret->setResponse(protocol::ThreadTeam::RetTryRunEC::DEMANDED);
        return Error::SUCCESS;
      }
    }else if(data.allocType == protocol::ThreadTeam::FORCE){
      if(nUsed > 0){
        //todo: use a more sophisticated mapping scheme
        auto pal = pa.get();
        ASSERT(pal);
        auto sce = pal->getSC(usedList[0]);
        auto r = ec->setSchedulingContext(sce);
        if(r){
          ret->setResponse(protocol::ThreadTeam::RetTryRunEC::FORCED);
          return Error::SUCCESS;
        }
      }
    }

    ret->setResponse(protocol::ThreadTeam::RetTryRunEC::FAILED);
    return Error::SUCCESS;
  }

  Error ThreadTeam::invokeRevokeDemand(Tasklet* /*t*/, Cap, IInvocation* msg){
    MLOG_DETAIL(mlog::pm, __func__);
    auto data = msg->getMessage()->read<protocol::ThreadTeam::RevokeDemand>();
    auto ece = msg->lookupEntry(data.ec());
    auto ret = msg->getMessage()->cast<protocol::ThreadTeam::RetRevokeDemand>();
    if(!ece){
      MLOG_ERROR(mlog::pm, "Error: Did not find EC!");
      return Error::INVALID_CAPABILITY;
    }

    TypedCap<ExecutionContext> ec(ece);
    if(ec && removeDemand(*ec)){
      ret->revoked = true;
    }else{
      MLOG_WARN(mlog::pm, "revoke demand failed");
      ret->revoked = false;
    }

    return Error::SUCCESS;
  }

  void ThreadTeam::notifyIdle(Tasklet* t, cpu::ThreadID id) {
    MLOG_DETAIL(mlog::pm, __func__, DVAR(id));
    monitor.request(t, [=](Tasklet*){
        if(!tryRunDemandAt(id)) {
          removeUsed(id);
          pushFree(id);
        }
        monitor.responseAndRequestDone();
    }); 
  }

  Error ThreadTeam::invokeRunNextToEC(Tasklet* /*t*/, Cap, IInvocation* /*msg*/){
    MLOG_ERROR(mlog::pm, __func__, " NYI!");
    return Error::NOT_IMPLEMENTED;
  }

  void ThreadTeam::pushFree(cpu::ThreadID id){
    MLOG_DETAIL(mlog::pm, __func__, DVAR(id));
    freeList[nFree] = id;
    nFree++;
  }

  optional<cpu::ThreadID> ThreadTeam::popFree(){
    MLOG_DETAIL(mlog::pm, __func__);
    optional<cpu::ThreadID> ret;
    if(nFree > 0){
      nFree--;
      ret = freeList[nFree];
    }
    return ret;
  }

  void ThreadTeam::pushUsed(cpu::ThreadID id){
    MLOG_DETAIL(mlog::pm, __func__, DVAR(id));
    usedList[nUsed] = id;
    nUsed++;
  }

  void ThreadTeam::removeUsed(cpu::ThreadID id){
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

  bool ThreadTeam::enqueueDemand(CapEntry* ec){
    if(nDemand < MYTHOS_MAX_THREADS){
      demandEC[demandList[nDemand]].set(this, ec, ec->cap());
      nDemand++;
      return true;
    }
    return false;
  }

  bool ThreadTeam::removeDemand(ExecutionContext* ec){
    //find entry
    for(unsigned i = 0; i < nDemand; i++){
      auto di = demandList[i];
      auto d = demandEC[di].get();
      if(d && *d == ec){
        demandEC[di].reset();
        nDemand--;
        //move following entries
        for(; i < nDemand; i++){
          demandList[i] = demandList[i+1];
        }
        //move demand index behind used indexes
        demandList[nDemand] = di;
        return true;
      }
    }
    MLOG_WARN(mlog::pm, "did not find EC in demand list ");
    return false;
  }

  //bool ThreadTeam::tryRunDemand() {
    //// demand available?
    //if(nDemand){
      ////take first
      //auto di = demandList[0];
      //TypedCap<ExecutionContext> ec(demandEC[di]);
      //ASSERT(ec);
      ////try to run ec
      //if(tryRun(*ec)){
        ////reset CapRef
        //demandEC[di].reset();
        //// remove from queue
        //nDemand--;
        //for(unsigned i = 0; i < nDemand; i++){
          //demandList[i] = demandList[i+1];
        //}
        //demandList[nDemand] = di;
        //return true;
      //}
    //}
    //return false;
  //}

  bool ThreadTeam::tryRunDemandAt(cpu::ThreadID id) {
    // demand available?
    if(nDemand){
      //take first
      auto di = demandList[0];
      TypedCap<ExecutionContext> ec(demandEC[di]);
      ASSERT(ec);
      //try to run ec
      if(tryRunAt(*ec, id)){
        //reset CapRef
        demandEC[di].reset();
        // remove from queue
        nDemand--;
        for(unsigned i = 0; i < nDemand; i++){
          demandList[i] = demandList[i+1];
        }
        demandList[nDemand] = di;
        return true;
      }
    }
    return false;
  }

  optional<ThreadTeam*> 
  ThreadTeamFactory::factory(CapEntry* dstEntry, CapEntry* memEntry, Cap memCap, IAllocator* mem, CapEntry* pae){
    auto obj = mem->create<ThreadTeam>();
    if (!obj) {
      dstEntry->reset();
      RETHROW(obj);
    }
    TypedCap<ProcessorAllocator> pa(pae); 
    if (!pa) RETHROW(pa);
    obj->pa.set(*obj, pae, pa.cap());
    Cap cap(*obj);
    auto res = cap::inherit(*memEntry, memCap, *dstEntry, cap);
    if (!res) {
      mem->free(*obj); // mem->release(obj) goes throug IKernelObject deletion mechanism
      RETHROW(res);
    }
    return *obj;
  }

} // namespace mythos
