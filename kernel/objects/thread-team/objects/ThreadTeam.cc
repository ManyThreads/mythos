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
    {}

  bool ThreadTeam::tryRunEC(ExecutionContext* ec){
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
    if(id){
      auto sce = pal->getSC(*id);
      TypedCap<SchedulingContext> sc(sce->cap());
      sc->registerThreadTeam(static_cast<INotifyIdle*>(this));
      pushUsed(*id);
      auto ret = ec->setSchedulingContext(sce);
      if(ret){
        MLOG_DETAIL(mlog::pm, "Init EC bind SC", DVAR(*id));
        return true;
      }else{
        MLOG_ERROR(mlog::pm, "ERROR: Init EC bind SC failed ", DVAR(*id));
      }
    }else{
      MLOG_ERROR(mlog::pm, "Cannot allocate SC for init EC");
    }
    return false;
  }

  Error ThreadTeam::invokeTryRunEC(Tasklet* /*t*/, Cap, IInvocation* msg){
    MLOG_ERROR(mlog::pm, __func__);
    
    auto data = msg->getMessage()->read<protocol::ThreadTeam::TryRunEC>();
    auto ece = msg->lookupEntry(data.ec());
    if(!ece){
      MLOG_ERROR(mlog::pm, "Error: Did not find EC!");
      return Error::INVALID_CAPABILITY;
    }

    TypedCap<ExecutionContext> ec(ece);

    if(ec && tryRunEC(*ec)){
      return Error::SUCCESS;
    }
    return Error::INSUFFICIENT_RESOURCES;
  }

  Error ThreadTeam::invokeDemandRunEC(Tasklet* /*t*/, Cap, IInvocation* /*msg*/){
    MLOG_ERROR(mlog::pm, __func__, " NYI!");
    return Error::NOT_IMPLEMENTED;
  }

  Error ThreadTeam::invokeForceRunEC(Tasklet* /*t*/, Cap, IInvocation* /*msg*/){
    MLOG_ERROR(mlog::pm, __func__, " NYI!");
    return Error::NOT_IMPLEMENTED;
  }

  Error ThreadTeam::invokeRunNextToEC(Tasklet* /*t*/, Cap, IInvocation* /*msg*/){
    MLOG_ERROR(mlog::pm, __func__, " NYI!");
    return Error::NOT_IMPLEMENTED;
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
