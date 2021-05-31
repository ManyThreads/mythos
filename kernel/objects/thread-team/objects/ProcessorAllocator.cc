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
 * Copyright 2020 Philipp Gypser, BTU Cottbus-Senftenberg
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

/* ProcessorAllocator */
  ProcessorAllocator::ProcessorAllocator()
      : nTeams(0)
      , tmpTeam(0)
      , nextTeam(0)
      , tmp_ret(nullptr)
    {}

  void ProcessorAllocator::init(){
    MLOG_DETAIL(mlog::pm, "PM::init");
    topology::systemTopo.init();
    for(size_t i = 0; i < topology::systemTopo.getNumSockets(); i++){
      auto socket = topology::systemTopo.getSocket(i);
      ASSERT(socket);
      socket->owner = this;
      lowLatencyFree.pushSocket(socket);
    }
  }

  bool ProcessorAllocator::tryReclaimLoop(Tasklet* t){
      MLOG_DETAIL(mlog::pm, __func__, DVAR(nextTeam), DVAR(tmpTeam));
      if(nextTeam == tmpTeam){
        return false;
      }
      ASSERT(nextTeam < nTeams);
      auto team = teamRefs[teamList[nextTeam]].get();
      ASSERT(team);
      if(reinterpret_cast<IResult<topology::Resource*>*>(*team) == tmp_ret){
        MLOG_DETAIL(mlog::pm, "skip allocating team");
        nextTeam = (nextTeam + 1) % nTeams;
        if(nextTeam == tmpTeam){
          MLOG_DETAIL(mlog::pm, "no other teams found");
          return false;
        }
        team = teamRefs[teamList[nextTeam]].get();
        ASSERT(team);
        if(reinterpret_cast<IResult<topology::Resource*>*>(*team) == tmp_ret){
            MLOG_DETAIL(mlog::pm, "same team registered twice?!");
            return false;
        }
      }
      nextTeam = (nextTeam + 1) % nTeams;
      team->reclaimResources(t, this);
      return true;
  }

  void ProcessorAllocator::alloc(Tasklet* t, IResult<topology::Resource*>* r){
    monitor.request(t,[=](Tasklet*){
      ASSERT(r);
      ASSERT(tmp_ret == nullptr);
      auto resource = lowLatencyFree.tryGetCoarseChunk(); 
      if(resource){
        r->response(t, resource);
      }else{
        tmp_ret = r;
        if(nextTeam >= nTeams){ nextTeam = 0;}
        ASSERT(nextTeam < nTeams);
        tmpTeam = nextTeam;
        if(tryReclaimLoop(t)){
          return;
        }
        r->response(t, optional<topology::Resource*>());
      }
      tmp_ret = nullptr;
      monitor.responseAndRequestDone();
    });
  }
  
  void ProcessorAllocator::free(Tasklet* t, topology::Resource* r){
    monitor.request(t,[=](Tasklet*){
      ASSERT(r);
      r->moveToPool(&lowLatencyFree);
      monitor.responseAndRequestDone();
    });
  }
} // namespace mythos
