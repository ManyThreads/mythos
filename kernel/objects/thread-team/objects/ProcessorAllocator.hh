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
#pragma once

#include "async/NestedMonitorDelegating.hh"
#include "objects/IFactory.hh"
#include "objects/IKernelObject.hh"
#include "cpu/hwthreadid.hh"
#include "objects/ProcessorTopology.hh"
#include "objects/ProcessorPool.hh"
#include "boot/mlog.hh"
#include "async/IResult.hh"

namespace mythos {

class ThreadTeam;

class ProcessorAllocator
  : public IKernelObject
  , public topology::IResourceOwner
  , public IResult<topology::Resource*>
{
  public:
    ProcessorAllocator();

  /* IKernelObject */
    optional<void> deleteCap(CapEntry&, Cap self, IDeleter& del) override;
    void deleteObject(Tasklet* t, IResult<void>* r) override;
    optional<void const*> vcast(TypeId id) const override {
      if (id == typeId<ProcessorAllocator>()) return this;
      THROW(Error::TYPE_MISMATCH);
    }

  /* IResult */
    //called from thread team
    void response(Tasklet* t, optional<topology::Resource*> r){
      monitor.response(t,[=](Tasklet*){
        ASSERT(tmp_ret);
        if(r){
          ASSERT(*r);
        }else{
          if(tryReclaimLoop(t)){
            monitor.responseDone();
            return;
          } 
        }
        tmp_ret->response(t, r);
        tmp_ret = nullptr;
        monitor.responseAndRequestDone();
      });
    }

  /* IResourceOwner */
    void notifyIdleThread(Tasklet* t, topology::IThread* thread) override {
      MLOG_INFO(mlog::pm, "Free thread notified idle...", DVARhex(t), DVARhex(thread));
    } 

    SleepMode getSleepMode() override { return DEEPSLEEP; }

    void init();

    //optional<cpu::ThreadID> alloc(); 
  
    //void free(Resource* r){
      //ASSERT(r);
      //r->moveToPool(&lowLatencyFree);
    //}
  
    //reclaim

    topology::Resource* alloc(){
      //return pool.tryGetCoarseChunk(); 
      return pool.getCore();
    } 

    bool tryReclaimLoop(Tasklet* t);
    void alloc(Tasklet* t, IResult<topology::Resource*>* r);
    void prealloc(Tasklet* t, IResult<topology::ICore*>* r);
    void free(Tasklet* t, topology::Resource* r, IResult<topology::ICore*>* result);

    void bind(optional<ThreadTeam*> /*tt*/){}
    void unbind(optional<ThreadTeam*> tt){
      MLOG_INFO(mlog::pm, "unregistered thread team", DVAR(tt));
      ASSERT(tt);
      for(unsigned i = 0; i < nTeams; i++){
        auto ti = teamList[i];
        auto t = teamRefs[ti].get();
        if(t && *t == *tt){
          nTeams--;
          for(; i < nTeams; i++){
            teamList[i] = teamList[i+1];
          }
        }
      }
    }

    void registerThreadTeam(Tasklet* t, CapEntry* threadTeam){
      monitor.request(t,[=](Tasklet*){
        if(nTeams < MAX_TEAMS){
          teamRefs[nTeams].set(this, threadTeam, threadTeam->cap());
          MLOG_INFO(mlog::pm, "registered thread team on index ", nTeams);
          nTeams++;
        }else{
          MLOG_INFO(mlog::pm, "ERROR: too many thread teams registered!");
        }
        monitor.responseAndRequestDone();
      });
    }

  private:
    static constexpr unsigned MAX_TEAMS = MYTHOS_MAX_THREADS;
    async::NestedMonitorDelegating monitor;
    //unsigned nFree;
    //cpu::ThreadID freeList[MYTHOS_MAX_THREADS];
    CapRef<ProcessorAllocator, ThreadTeam> teamRefs[MAX_TEAMS];
    unsigned teamList[MAX_TEAMS];
    unsigned nTeams;

    unsigned tmpTeam, nextTeam;
    IResult<topology::Resource*>* tmp_ret;
    
    ProcessorPool pool;
};
} // namespace mythos
