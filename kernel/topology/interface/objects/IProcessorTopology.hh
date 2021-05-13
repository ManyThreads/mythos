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

#include "cpu/hwthreadid.hh"
#include "objects/IProcessorPool.hh"
#include "boot/mlog.hh"
#include "objects/SchedulingContext.hh"

namespace mythos {
namespace topology {
 
  struct ISocket;
  struct ITile;
  struct ICore;
  struct IThread;

  struct IResourceOwner {
    virtual void notifyIdleThread(Tasklet* t, IThread* thread) = 0;
  };

  struct Resource{
    //enum State{
      //FREE,
      //PARTITIONED,
      //ALLOCATED
    //};
    //State state = FREE;
    IResourceOwner* owner = nullptr;

    virtual IResourceOwner* getOwner() = 0;
    virtual void moveToPool(IProcessorPool* pool) = 0;
    virtual void setOwner(IResourceOwner* ro) = 0;
  };


  template<class TYPE>
  struct Queueable{
    TYPE* next = nullptr;
  };


  struct ISystem {
    virtual size_t getNumSockets() = 0;
    virtual ISocket* getSocket(size_t idx) = 0;
  };

  struct ISocket : public Resource, public Queueable<ISocket> {
    virtual size_t getNumTiles() = 0;
    virtual ITile* getTile(size_t idx) = 0;
    virtual ISystem* getSystem() = 0;
    IResourceOwner* getOwner() override {
      if(owner){
        return owner;
      }else{
        MLOG_ERROR(mlog::pm, __func__, "ERROR: resource owner not found!");
        return nullptr;
      }
    }

    void moveToPool(IProcessorPool* p) override {
      ASSERT(p);
      p->pushSocket(this);
    }

    void setOwner(IResourceOwner* ro) override {
      MLOG_INFO(mlog::pm, __func__, DVARhex(ro));
      if(owner){
        MLOG_INFO(mlog::pm, __func__, "replace owner");
        owner = ro;
      }else{
        MLOG_INFO(mlog::pm, __func__, "split ownership");
        ASSERT(getOwner() != ro);
        owner = ro;
      }
    }
  };

  struct ITile : public Resource, public Queueable<ITile> {
    virtual size_t getNumCores() = 0;
    virtual ICore* getCore(size_t idx) = 0;
    virtual ISocket* getSocket() = 0;
    IResourceOwner* getOwner() override {
      if(owner){
        return owner;
      }else{
        auto parent = getSocket();
        ASSERT(parent);
        return parent->getOwner();
      }
    }

    void moveToPool(IProcessorPool* p) override {
      ASSERT(p);
      p->pushTile(this);
    }

    void setOwner(IResourceOwner* ro) override {
      MLOG_INFO(mlog::pm, __func__, DVARhex(ro));
      auto socket = getSocket();
      if(owner){
        MLOG_INFO(mlog::pm, __func__, "replace owner");
        ASSERT(socket->owner == nullptr);
        owner = ro;
        //try to merge
        MLOG_INFO(mlog::pm, __func__, "try to  merge");
        for(size_t i = 0; i < socket->getNumTiles(); i++){
          auto t = socket->getTile(i);
          if(t->owner != ro){
            MLOG_INFO(mlog::pm, __func__, "merge failed");
            return;
          }
        }
        for(size_t i = 0; i < socket->getNumTiles(); i++){
          auto t = socket->getTile(i);
          t->owner = nullptr;
        }
        MLOG_INFO(mlog::pm, __func__, "merge!");
        socket->setOwner(ro);
      }else{
        MLOG_INFO(mlog::pm, __func__, "split ownership");
        ASSERT(getOwner() != ro);
        owner = ro;
        //split tree
        for(size_t i = 0; i < socket->getNumTiles(); i++){
          auto t = socket->getTile(i);
          auto o = socket->getOwner();
          if(t != this){
            ASSERT(t->owner == nullptr);
            t->owner = o;
          }
        }
        socket->setOwner(nullptr);
      }
    }
  };

  struct ICore : public Resource, public Queueable<ICore> {
    virtual size_t getNumThreads() = 0;
    virtual IThread* getThread(size_t idx) = 0;
    virtual ITile* getTile() = 0;
    IResourceOwner* getOwner() override {
      if(owner){
        return owner;
      }else{
        auto parent = getTile();
        ASSERT(parent);
        return parent->getOwner();
      }
    }

    void moveToPool(IProcessorPool* p) override {
      ASSERT(p);
      p->pushCore(this);
    }

    void setOwner(IResourceOwner* ro) override {
      MLOG_INFO(mlog::pm, __func__, DVARhex(ro));
      auto tile = getTile();
      if(owner){
        MLOG_INFO(mlog::pm, __func__, "replace owner");
        ASSERT(tile->owner == nullptr);
        owner = ro;
        //try to merge
        MLOG_INFO(mlog::pm, __func__, "try to  merge");
        for(size_t i = 0; i < tile->getNumCores(); i++){
          auto c = tile->getCore(i);
          if(c->owner != ro){
            MLOG_INFO(mlog::pm, __func__, "merge failed");
            return;
          }
        }
        for(size_t i = 0; i < tile->getNumCores(); i++){
          auto c = tile->getCore(i);
          c->owner = nullptr;
        }
        MLOG_INFO(mlog::pm, __func__, "merge!");
        tile->setOwner(ro);
      }else{
        MLOG_INFO(mlog::pm, __func__, "split ownership");
        ASSERT(getOwner() != ro);
        owner = ro;
        //split tree
        for(size_t i = 0; i < tile->getNumCores(); i++){
          auto c = tile->getCore(i);
          auto o = tile->getOwner();
          if(c != this){
            ASSERT(c->owner == nullptr);
            c->owner = o;
          }
        }
        tile->setOwner(nullptr);
      }
    }
  };

  struct IThread : public Resource, public Queueable<IThread> {
    virtual ICore* getCore() = 0;
    virtual cpu::ThreadID getThreadID() = 0;
    virtual cpu::ApicID getApicID() = 0;
    virtual CapEntry* getSC() = 0;
    IResourceOwner* getOwner() override {
      if(owner){
        return owner;
      }else{
        auto parent = getCore();
        ASSERT(parent);
        return parent->getOwner();
      }
    }

    void moveToPool(IProcessorPool* p) override {
      ASSERT(p);
      p->pushThread(this);
    }

    void setOwner(IResourceOwner* ro) override {
      MLOG_INFO(mlog::pm, __func__, DVARhex(ro));
      auto core = getCore();
      if(owner){
        MLOG_INFO(mlog::pm, __func__, "replace owner");
        ASSERT(core->owner == nullptr);
        owner = ro;
        //try to merge
        MLOG_INFO(mlog::pm, __func__, "try to  merge");
        for(size_t i = 0; i < core->getNumThreads(); i++){
          auto t = core->getThread(i);
          if(t->owner != ro){
            MLOG_INFO(mlog::pm, __func__, "merge failed");
            return;
          }
        }
        for(size_t i = 0; i < core->getNumThreads(); i++){
          auto t = core->getThread(i);
          t->owner = nullptr;
        }
        MLOG_INFO(mlog::pm, __func__, "merge!");
        core->setOwner(ro);
      }else{
        MLOG_INFO(mlog::pm, __func__, "split ownership");
        ASSERT(getOwner() != ro);
        owner = ro;
        //split tree
        for(size_t i = 0; i < core->getNumThreads(); i++){
          auto t = core->getThread(i);
          auto o = core->getOwner();
          if(t != this){
            ASSERT(t->owner == nullptr);
            t->owner = o;
          }
        }
        core->setOwner(nullptr);
      }
    }
  };

  struct Thread : public IThread, public INotifyIdle
  {
    ICore* getCore() override { return core; }
    cpu::ThreadID getThreadID() override { return threadID; }
    cpu::ApicID getApicID() override { return apicID; }
    ICore* core = nullptr;
    cpu::ThreadID threadID = 0;
    cpu::ApicID apicID = 0;

    void init(cpu::ThreadID tID){
      this->threadID = tID;
      this->apicID = boot::getScheduler(tID).getHome()->getApicID();
      MLOG_INFO(mlog::pm, __func__, DVAR(threadID), DVAR(apicID));
      getSC()->initRoot(Cap(image2kernel(&boot::getScheduler(tID))));
      boot::getScheduler(tID).registerIdleNotify(this);
    }
      
    void notifyIdle() override {
      MLOG_INFO(mlog::pm, __func__, DVAR(threadID), DVAR(apicID));
      auto o = getOwner();
      ASSERT(o);
      o->notifyIdleThread(&tasklet, this);
    }

    CapEntry* getSC() override { return image2kernel(&mySC); }
    CapEntry mySC;
    Tasklet tasklet; //task for communication with processor allocator
  };
  
  template<size_t THREADS>
  struct FixedCore : public ICore
  {
    FixedCore(){
      for(size_t i = 0; i < THREADS; i++){
        threads[i].core = this;
      }
    }

    size_t getNumThreads() override { return THREADS; };
    IThread* getThread(size_t idx) override {
      ASSERT(idx < THREADS);
      return &threads[idx];
    }
    ITile* getTile() override { return tile; }

    ITile* tile = nullptr;
    Thread threads[THREADS];
  };
  
  template<size_t CORES, size_t THREADS>
  struct FixedTile : public ITile
  {
    FixedTile(){
      for(size_t i = 0; i < CORES; i++){
        cores[i].tile = this;
      }
    }

    size_t getNumCores() override { return CORES; }
    ICore* getCore(size_t idx) override {
      ASSERT(idx < CORES);
      return &cores[idx];
    }

    ISocket* getSocket() override { return socket; }

    ISocket* socket = nullptr;
    FixedCore<THREADS> cores[CORES]; 
  };

  template<size_t TILES, size_t CORES, size_t THREADS>
  struct FixedSocket : public ISocket
  {
    FixedSocket(){
      for(size_t i = 0; i < TILES; i++){
        tiles[i].socket = this;
      }
    }

    size_t getNumTiles() override { return TILES; }
    ITile* getTile(size_t idx) override {
      ASSERT(idx < TILES);
      return &tiles[idx];
    }
    ISystem* getSystem() { return system; }

    ISystem* system = nullptr;
    FixedTile<CORES, THREADS> tiles[TILES];
  };

  template<size_t SOCKETS, size_t TILES, size_t CORES, size_t THREADS>
  struct FixedSystem : public ISystem
  {
    FixedSystem(){
      for(size_t i = 0; i < SOCKETS; i++){
        sockets[i].system = this;
      }
    }

    size_t getNumSockets() { return SOCKETS; }
    ISocket* getSocket(size_t idx) override {
      ASSERT(idx < SOCKETS);
      return &sockets[idx];
    }

    FixedSocket<TILES, CORES, THREADS> sockets[SOCKETS];
  };
} //topology
} // namespace mythos
