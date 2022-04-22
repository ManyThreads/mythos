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
#include "objects/PerformanceMonitoring.hh"

namespace mythos {

/* IKernelObject */ 
  optional<void> ThreadTeam::deleteCap(CapEntry&, Cap self, IDeleter& del)
  {
    MLOG_DETAIL(mlog::pm, __func__);
    ASSERT(state == IDLE);
    if (self.isOriginal()) { // the object gets deleted, not a capability reference
      ASSERT(pa);

      // free used SCs
      //auto used = popUsed();
      //while(used){
        //auto sce = pa->getSC(*used);
        //TypedCap<SchedulingContext> sc(sce->cap());
        //sc->resetThreadTeam();
        //todo: synchronize!!!!
        //pa->free(*used);
        //numAllocated--;
        //used = popUsed();
      //}

      // free unused SCs
      auto free = getFree();
      while(free){
        //auto sce = pa->getSC(*free);
        //TypedCap<SchedulingContext> sc(sce->cap());
        //sc->resetThreadTeam();
        //todo:: synchronize!!!
        //pa->free(*free);
        //numAllocated--;
        free = getFree();
      }
      del.deleteObject(del_handle);
    }
    RETURN(Error::SUCCESS);
  }

  void ThreadTeam::deleteObject(Tasklet* t, IResult<void>* r)
  {
    MLOG_DETAIL(mlog::pm, __func__);
    ASSERT(state == IDLE);
    monitor.doDelete(t, [=](Tasklet* t) { this->memory->free(t, r, this, sizeof(ThreadTeam)); });
  }

  void ThreadTeam::invoke(Tasklet* t, Cap self, IInvocation* msg)
  {
    MLOG_DETAIL(mlog::pm, __func__, DVAR(t), DVAR(msg));
    monitor.request(t, [=](Tasklet* /*t*/){
        ASSERT(state == IDLE);
        state = INVOCATION;
        tmp_msg = msg;
        Error err = Error::NOT_IMPLEMENTED;
        switch (msg->getProtocol()) {
        case protocol::ThreadTeam::proto:
          err = protocol::ThreadTeam::dispatchRequest(this, msg->getMethod(), t, self, msg);
          break;
        }
        if (err != Error::INHIBIT) {
          ASSERT(state == INVOCATION);
          state = IDLE;
          tmp_msg = nullptr;
          msg->replyResponse(err);
          monitor.requestDone();
        }
      } );
  }

/* IResult */
  void ThreadTeam::response(Tasklet* t, optional<topology::Resource*> r){
    MLOG_DETAIL(mlog::pm, __PRETTY_FUNCTION__);
    monitor.response(t, [=](Tasklet* t){
      ASSERT(pa);
      ASSERT(tmp_msg);
      ASSERT(tmp_msg->getMethod() == protocol::ThreadTeam::TRYRUNEC);

      auto data = tmp_msg->getMessage()->read<protocol::ThreadTeam::TryRunEC>();
      auto ece = tmp_msg->lookupEntry(data.ec());

      ASSERT(ece);

      if(r && *r){
        MLOG_DETAIL(mlog::pm, DVARhex(*r));
        r->setOwner(this);
        r->moveToPool(&freePool);
        auto thread = getFree();
        if(thread){
          numAllocated++;
          TypedCap<ExecutionContext> ec(ece);
          ASSERT(ec);
          tryRunAt(t, *ec, *thread);
          monitor.responseDone();
          return;
        }
      }
    
      MLOG_DETAIL(mlog::pm, "SC allocation failed");

      auto ret = tmp_msg->getMessage()->write<protocol::ThreadTeam::RetTryRunEC>();

      if(data.allocType == protocol::ThreadTeam::DEMAND){
        if(enqueueDemand(*ece)){
          MLOG_ERROR(mlog::pm, "enqueued to demand list", DVARhex(*ece));
          ret->setResponse(protocol::ThreadTeam::RetTryRunEC::DEMANDED);
        }else{
          ret->setResponse(protocol::ThreadTeam::RetTryRunEC::FAILED);
        }
      }else if(data.allocType == protocol::ThreadTeam::FORCE){
        MLOG_ERROR(mlog::pm, "force run NYI!");
        ret->setResponse(protocol::ThreadTeam::RetTryRunEC::FAILED);
        //if(nUsed > 0){
          //MLOG_DETAIL(mlog::pm, "force run");
          ////todo: use a more sophisticated mapping scheme
          //ret->setResponse(protocol::ThreadTeam::RetTryRunEC::FORCED);
          //TypedCap<ExecutionContext> ec(ece);
          //ASSERT(ec);
          //tryRunAt(t, *ec, usedList[0]);
          //return;
        //}
      }

    //if(id){
      //MLOG_DETAIL(mlog::pm, DVAR(*id));
      //numAllocated++;
      //auto sce = pa->getSC(*id);
      //TypedCap<SchedulingContext> sc(sce->cap());
      //sc->registerThreadTeam(this);
      //perfmon::initAt(*id);
      //perfmon::printAt(*id);

      state = IDLE;
      tmp_msg->replyResponse(Error::SUCCESS);
      tmp_msg = nullptr;
      monitor.responseAndRequestDone();
    });
  }

  void ThreadTeam::response(Tasklet* t, optional<topology::ICore*> r){
    MLOG_INFO(mlog::pm, __PRETTY_FUNCTION__, DVARhex(t));
    monitor.response(t, [=](Tasklet* t){
      if(r && *r){
        MLOG_DETAIL(mlog::pm, DVARhex(*r));
        r->setOwner(this);
        r->moveToPool(&freePool);
      }
      if(tmp_msg){
        tmp_msg->replyResponse(Error::SUCCESS);
        tmp_msg = nullptr;
      }
      monitor.responseAndRequestDone();
    });
  }

  void ThreadTeam::response(Tasklet* t, optional<void> bound){
    MLOG_DETAIL(mlog::pm, __PRETTY_FUNCTION__);
    monitor.response(t, [=](Tasklet* /*t*/){
      ASSERT(tmp_thread);

      if(state == INVOCATION){
        MLOG_DETAIL(mlog::pm, "state = INVOCATION");
        ASSERT(tmp_msg);
        ASSERT(tmp_msg->getMethod() == protocol::ThreadTeam::TRYRUNEC);

        auto ret = tmp_msg->getMessage()->write<protocol::ThreadTeam::RetTryRunEC>();

        if(bound){
          MLOG_INFO(mlog::pm, "EC successfully bound to SC");
          //pushUsed(tmp_id);
          //allready set! ret->setResponse(protocol::ThreadTeam::RetTryRunEC::ALLOCATED);
          ret->setResponse(protocol::ThreadTeam::RetTryRunEC::ALLOCATED);
          if(balancePools(t)){
            state = IDLE;
            tmp_thread = nullptr;
            monitor.responseDone();
            return;
          }
          tmp_msg->replyResponse(Error::SUCCESS);
          tmp_msg = nullptr;
        }else{
          MLOG_ERROR(mlog::pm, "ERROR: failed to bind EC to SC!");
          //todo: reuse sc?
          ret->setResponse(protocol::ThreadTeam::RetTryRunEC::FAILED);
          tmp_msg->replyResponse(Error::GENERIC_ERROR);
          tmp_msg = nullptr;
        }
      }else if(state == SC_NOTIFY){
        MLOG_DETAIL(mlog::pm, "state = SC_NOTIFY");
        if(bound){
          MLOG_INFO(mlog::pm, "EC successfully bound to SC");
          // remove ec from demand queue
          removeDemand(tmp_ec, true);
        }else{
          //removeUsed(tmp_id);
          //pushFree(tmp_id);
          cachedPool.pushThread(tmp_thread);
        }
      }else{
        MLOG_ERROR(mlog::pm, "ERROR: Invalid operational state");
      }

      state = IDLE;
      tmp_thread = nullptr;
      MLOG_INFO(mlog::pm, "bound", DVARhex(t));
      monitor.responseAndRequestDone();
    });
  }

/* ThreadTeam */
  ThreadTeam::ThreadTeam(IAsyncFree* memory)
    : memory(memory)
    , tmp_msg(nullptr)
    , tmp_thread(nullptr)
    , tmp_ec(nullptr)
    , rm_ec(nullptr)
    , state(IDLE)
    , pa(nullptr)
    //, nFree(0)
    //, nUsed(0)
    , nDemand(0)
    , limit(0)
    , numAllocated(0)
    {
      for(unsigned d = 0; d < MYTHOS_MAX_THREADS; d++){
        demandList[d] = d;
      }
    }

  bool ThreadTeam::tryRun(ExecutionContext* ec){
    ASSERT(state == IDLE);
    ASSERT(pa);
    ASSERT(ec);

    auto thread = getFree();
    if(!thread && !limitReached()){
      auto r = pa->alloc();
      if(r){
        r->setOwner(this);
        r->moveToPool(&freePool);
        thread = getFree();
      }
    }

    if(thread){
      if(ec->setSchedulingContext(thread->getSC())){
        //pushUsed(*id);
        numAllocated++;
        return true;
      }else{
        (*thread)->moveToPool(&freePool);
      }
    }
    return false;
  }

  void ThreadTeam::bind(optional<ProcessorAllocator*> paPtr) {
    MLOG_DETAIL(mlog::pm, "bind processor allocator");
    //todo:: synchronize
    if(paPtr){
      pa = *paPtr;
    }else{
      MLOG_ERROR(mlog::pm, "ERROR: binding processor allocator failed");
      pa = nullptr;
    }
  }

  void ThreadTeam::unbind(optional<ProcessorAllocator*> ) {
    MLOG_ERROR(mlog::pm, "ERROR: unbind processor allocator");
    //todo:: synchronize
    pa = nullptr;
  }

  void ThreadTeam::tryRunAt(Tasklet* t, ExecutionContext* ec, topology::IThread* thread){
    MLOG_INFO(mlog::pm, __func__, DVARhex(ec), DVARhex(thread), DVAR(thread->getThreadID()), DVARhex(t));
    ASSERT(thread);
    tmp_thread = thread;
    auto sce = thread->getSC();
    ec->setSchedulingContext(t, this, sce);
  }

  Error ThreadTeam::invokeTryRunEC(Tasklet* t, Cap, IInvocation* msg){
    MLOG_INFO(mlog::pm, __func__);
    ASSERT(state == INVOCATION);
    
    auto data = msg->getMessage()->read<protocol::ThreadTeam::TryRunEC>();
    auto ece = msg->lookupEntry(data.ec());

    if(!ece){
      MLOG_ERROR(mlog::pm, "Error: Did not find EC!");
      auto ret = msg->getMessage()->write<protocol::ThreadTeam::RetTryRunEC>();
      ret->setResponse(protocol::ThreadTeam::RetTryRunEC::FAILED);
      return Error::INVALID_CAPABILITY;
    }
    
    auto thread = getFree();

    if(thread){
      MLOG_DETAIL(mlog::pm, "take SC from Team ", DVAR(thread->getThreadID()));
      TypedCap<ExecutionContext> ec(ece);
      ASSERT(ec);
      numAllocated++;
      tryRunAt(t, *ec, *thread);
    }else if(limitReached()){
      MLOG_DETAIL(mlog::pm, "limit reached!");
      response(t, optional<topology::Resource*>());
    }else{
      MLOG_DETAIL(mlog::pm, "try alloc SC from PA");
      ASSERT(pa);
      pa->alloc(t, this);
    }
    
    return Error::INHIBIT;
  }
    
  Error ThreadTeam::invokeRevokeDemand(Tasklet* /*t*/, Cap, IInvocation* msg){
    MLOG_INFO(mlog::pm, __func__);
    ASSERT(state == INVOCATION);

    auto data = msg->getMessage()->read<protocol::ThreadTeam::RevokeDemand>();
    auto ece = msg->lookupEntry(data.ec());
    auto ret = msg->getMessage()->cast<protocol::ThreadTeam::RetRevokeDemand>();

    ret->revoked = false;

    if(!ece){
      MLOG_ERROR(mlog::pm, "Error: Did not find EC!");
      return Error::INVALID_CAPABILITY;
    }

    TypedCap<ExecutionContext> ec(ece);
    if(ec && removeDemand(*ec, true)){
      ret->revoked = true;
    }else{
      MLOG_INFO(mlog::pm, "revoke demand failed");
    }
    return Error::SUCCESS;
  }

  Error ThreadTeam::invokeSetLimit(Tasklet* /*t*/, Cap, IInvocation* msg){
    MLOG_INFO(mlog::pm, __func__);
    ASSERT(state == INVOCATION);

    auto data = msg->getMessage()->read<protocol::ThreadTeam::SetLimit>();
    auto oldLimit = limit;
    limit = data.limit;
    MLOG_ERROR(mlog::pm, DVAR(oldLimit), DVAR(limit));

    return Error::SUCCESS;
  }

  Error ThreadTeam::invokeResetPerfMon(Tasklet* /*t*/, Cap, IInvocation* msg){
    MLOG_INFO(mlog::pm, __func__);
    ASSERT(state == INVOCATION);
    //for(unsigned i = 0; i < nUsed; i++){
      //auto id = usedList[i];
      //perfmon::resetAt(id);
    //}

    //for(unsigned i = 0; i < MYTHOS_MAX_THREADS; i++){
      //pmm[i].reset();
    //}

    return Error::SUCCESS;
  }

  Error ThreadTeam::invokePrintPerfMon(Tasklet* /*t*/, Cap, IInvocation* msg){
    MLOG_INFO(mlog::pm, __func__);
    ASSERT(state == INVOCATION);
    //for(unsigned i = 0; i < nUsed; i++){
      //auto id = usedList[i];
      //perfmon::readAt(id, &pmm[i]);
    //}

    //uint64_t sumTicks = 0;
    //uint64_t sumUnhalted = 0;
    //for(unsigned i = 0; i < MYTHOS_MAX_THREADS; i++){
      //if(pmm[i].tscTicks){
        //auto ticks = pmm[i].tscTicks;
        //sumTicks += ticks;
        //auto unhalted = pmm[i].refCyclesUnhalted;
        //sumUnhalted += unhalted;
        //auto utilization = (unhalted * 100) / ticks;
        //MLOG_ERROR(mlog::pm, "SC=", i, DVAR(ticks), DVAR(unhalted), DVAR(utilization));
      //}
    //}
    //auto avgUtilization = (sumUnhalted * 100) / sumTicks;
    //MLOG_ERROR(mlog::pm, DVAR(sumTicks), DVAR(sumUnhalted), DVAR(avgUtilization));

    return Error::SUCCESS;
  }

  Error ThreadTeam::invokeRunNextToEC(Tasklet* /*t*/, Cap, IInvocation* /*msg*/){
    MLOG_ERROR(mlog::pm, __func__, " NYI!");
    return Error::NOT_IMPLEMENTED;
  }

  //void ThreadTeam::pushFree(cpu::ThreadID id){
    //MLOG_DETAIL(mlog::pm, __func__, DVAR(id));
    //freeList[nFree] = id;
    //nFree++;
  //}

  optional<topology::IThread*> ThreadTeam::getFree(){
    MLOG_DETAIL(mlog::pm, __func__);
    optional<topology::IThread*> ret;
    if(!limitReached()){
      //try get from cached pool
      auto thread = cachedPool.getThread();
      if(!thread){
        //try get from free pool
        thread = freePool.getThread();
      }
      if(thread){
        ret = thread;
      }
    }else{
      MLOG_DETAIL(mlog::pm, __func__, " limit reached!");
    }

    return ret;
  }

  //void ThreadTeam::pushUsed(cpu::ThreadID id){
    //MLOG_DETAIL(mlog::pm, __func__, DVAR(id));
    //usedList[nUsed] = id;
    //nUsed++;
  //}

  //optional<cpu::ThreadID> ThreadTeam::popUsed(){
    //MLOG_DETAIL(mlog::pm, __func__);
    //optional<cpu::ThreadID> ret;
    //if(nUsed > 0){
      //nUsed--;
      //ret = usedList[nUsed];
    //}
    //return ret;
  //}

  //void ThreadTeam::removeUsed(cpu::ThreadID id){
    //MLOG_DETAIL(mlog::pm, __func__, DVAR(id));
    //for(unsigned i = 0; i < nUsed; i++){
      //if(usedList[i] == id){
        //nUsed--;
        //for(; i < nUsed; i++){
          //usedList[i] = usedList[i+1];
        //}
        //return;
      //}
    //}
    //MLOG_ERROR(mlog::pm, "ERROR: did not find used ThreadID ", id);
  //}

  bool ThreadTeam::enqueueDemand(CapEntry* ec){
    MLOG_DETAIL(mlog::pm, __func__, DVARhex(ec));
    ASSERT(state == INVOCATION);
    if(nDemand < MYTHOS_MAX_THREADS){
      demandEC[demandList[nDemand]].set(this, ec, ec->cap());
      nDemand++;
      //dumpDemand();
      return true;
    }
    return false;
  }

  bool ThreadTeam::removeDemand(ExecutionContext* ec, bool resetRef){
    MLOG_DETAIL(mlog::pm, __func__, DVARhex(ec), DVAR(resetRef));
    //find entry
    for(unsigned i = 0; i < nDemand; i++){
      auto di = demandList[i];
      auto d = demandEC[di].get();
      if(d && *d == ec){
        nDemand--;
        //move following entries
        for(; i < nDemand; i++){
          demandList[i] = demandList[i+1];
        }
        //move demand index behind used indexes
        demandList[nDemand] = di;
        //reset entry
        if(resetRef){
          rm_ec = *d;
          demandEC[di].reset();
        }
        //dumpDemand();
        return true;
      }
    }
    MLOG_INFO(mlog::pm, "did not find EC in demand list ");
    //dumpDemand();
    return false;
  }

  //void ThreadTeam::notifyIdle(Tasklet* t, cpu::ThreadID id) {
    //MLOG_INFO(mlog::pm, __func__, DVAR(id));
    //monitor.request(t, [=](Tasklet*){
        //ASSERT(state == IDLE);
        //ASSERT(tmp_id == INV_ID);
        //state = SC_NOTIFY;

        //if(numAllocated > limit || !tryRunDemandAt(t, id)) {
          //removeUsed(id);
          ////pushFree(id);
          //ASSERT(pa);
          //state = IDLE;
          ////pa->free(t, id);
          //numAllocated--;
          //monitor.responseAndRequestDone();
        //}
    //}); 
  //}

  bool ThreadTeam::tryRunDemandAt(Tasklet* t, topology::IThread* thread) {
    MLOG_DETAIL(mlog::pm, __func__);
    ASSERT(state == SC_NOTIFY);
    ASSERT(thread);
    // demand available?
    if(nDemand){
      //take first
      auto di = demandList[0];
      TypedCap<ExecutionContext> ec(demandEC[di]);
      ASSERT(ec);
      MLOG_DETAIL(mlog::pm, DVARhex(*ec), DVAR(thread->getThreadID()));
      tmp_ec = *ec;
      //try to run ec
      tryRunAt(t, *ec, thread);
      return true;
    }
    //dumpDemand();
    return false;
  }

  void ThreadTeam::bind(optional<ExecutionContext*> /*ec*/){}

  void ThreadTeam::unbind(optional<ExecutionContext*> ec){
    MLOG_DETAIL(mlog::pm, __func__, DVARhex(ec));
    if(ec){
      if(*ec == rm_ec){
        MLOG_DETAIL(mlog::pm, "synchronous unbind");
        rm_ec = nullptr;
      }else{
        MLOG_DETAIL(mlog::pm, "asynchronous unbind");
        //todo: tasklet valid after unbind?
        auto ecPtr = *ec;
        monitor.request(&ec->threadTeamTasklet, [=](Tasklet*){
            ASSERT(state == IDLE);
            removeDemand(ecPtr, false);
            monitor.responseAndRequestDone();
        }); 
      }
    }
  }

  void ThreadTeam::dumpDemand(){
    MLOG_DETAIL(mlog::pm, "demand list:");
    for(unsigned i = 0; i < MYTHOS_MAX_THREADS; i++){
      auto di = demandList[i];
      if(i < nDemand){
        auto d = demandEC[di].get();
        ASSERT(d);
        MLOG_DETAIL(mlog::pm, DVAR(i), DVAR(di), DVARhex(*d));
      }else{
        if(i != di) MLOG_DETAIL(mlog::pm, DVAR(i), DVAR(di)," slot free");
      }
    }
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
    obj->paRef.set(*obj, pae, pa.cap());
    Cap cap(*obj);
    auto res = cap::inherit(*memEntry, memCap, *dstEntry, cap);
    if (!res) {
      mem->free(*obj); // mem->release(obj) goes throug IKernelObject deletion mechanism
      RETHROW(res);
    }
    pa->registerThreadTeam(&obj->paTasklet, dstEntry);
    return *obj;
  }

} // namespace mythos
