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

#include "objects/IProcessorTopology.hh"
#include "objects/IProcessorPool.hh"

namespace mythos {

  struct ProcessorPool : public IProcessorPool {
    
    void pushSocket(topology::ISocket* s) override { 
      ASSERT(s);
      ASSERT(s->next == nullptr);
      s->next = sockets;
      sockets = s;
    }

    void pushTile(topology::ITile* t) override {
      ASSERT(t);
      ASSERT(t->next == nullptr);
      t->next = tiles;
      tiles = t;
      
      // only merge if whole core has same owner
      if(!t->owner){
        auto socket = t->getSocket();
        ASSERT(socket);
        auto numTiles = socket->getNumTiles();
        for(size_t i = 0; i < numTiles; i++){
          auto curr = tiles;
          while(curr != socket->getTile(i)){
            if(!curr){ 
              MLOG_DETAIL(mlog::pm, __func__, "merge failed");
              return; // merge failed
            }
            curr = curr->next;
          }
        }

        //merge
        auto prev = &tiles;
        auto curr = tiles;
        while(numTiles > 0){
          ASSERT(curr);
          while(curr->getSocket() != socket){
            prev = &curr->next;
            curr = curr->next;
            ASSERT(curr);
          }
          *prev = curr->next;
          curr->next = nullptr;
          curr = *prev; 
          numTiles--;
        }
        MLOG_DETAIL(mlog::pm, __func__, "merged");
        pushSocket(socket);
      }
    }

    void pushCore(topology::ICore* c) override {
      ASSERT(c);
      ASSERT(c->next == nullptr);
      c->next = cores;
      cores = c;
      
      // only merge if whole core has same owner
      if(!c->owner){
        auto tile = c->getTile();
        ASSERT(tile);
        auto numCores = tile->getNumCores();
        for(size_t i = 0; i < numCores; i++){
          auto curr = cores;
          while(curr != tile->getCore(i)){
            if(!curr){ 
              MLOG_DETAIL(mlog::pm, __func__, "merge failed");
              return; // merge failed
            }
            curr = curr->next;
          }
        }

        //merge
        auto prev = &cores;
        auto curr = cores;
        while(numCores > 0){
          ASSERT(curr);
          while(curr->getTile() != tile){
            prev = &curr->next;
            curr = curr->next;
            ASSERT(curr);
          }
          *prev = curr->next;
          curr->next = nullptr;
          curr = *prev; 
          numCores--;
        }
        MLOG_DETAIL(mlog::pm, __func__, "merged");
        pushTile(tile);
      }
    }

    void pushThread(topology::IThread* t) override {
      ASSERT(t);
      ASSERT(t->next == nullptr);
      t->next = threads;
      threads = t;

      // only merge if whole core has same owner
      if(!t->owner){
        auto core = t->getCore();
        ASSERT(core);
        auto numThreads = core->getNumThreads();
        for(size_t i = 0; i < numThreads; i++){
          auto curr = threads;
          while(curr != core->getThread(i)){
            if(!curr){
              MLOG_DETAIL(mlog::pm, __func__, "merge failed");
              return; // merge failed
            } 
            curr = curr->next;
          }
        }

        //merge
        auto prev = &threads;
        auto curr = threads;
        while(numThreads > 0){
          ASSERT(curr);
          while(curr->getCore() != core){
            prev = &curr->next;
            curr = curr->next;
            ASSERT(curr);
          }
          *prev = curr->next;
          curr->next = nullptr;
          curr = *prev; 
          numThreads--;
        }
        MLOG_DETAIL(mlog::pm, __func__, "merged");
        pushCore(core);
      }
    }
    
    topology::ISocket* popSocket() override { 
      if(sockets){
        auto ret = sockets;
        sockets = sockets->next;
        ret->next = nullptr;
        return ret;
      } 
      return nullptr;
    }

    topology::ITile* popTile() override { 
      if(tiles){
        auto ret = tiles;
        tiles = tiles->next;
        ret->next = nullptr;
        return ret;
      } 
      return nullptr;
    }

    topology::ICore* popCore() override { 
      if(cores){
        auto ret = cores;
        cores = cores->next;
        ret->next = nullptr;
        return ret;
      } 
      return nullptr;
    }

    topology::IThread* popThread() override { 
      if(threads){
        auto ret = threads;
        threads = threads->next;
        ret->next = nullptr;
        return ret;
      } 
      return nullptr;
    }

    topology::Resource* tryGetCoarseChunk() override {
      if(sockets){ return popSocket(); }
      if(tiles){ return popTile(); }
      if(cores){ return popCore(); }
      if(threads){ return popThread(); }
      return nullptr;
    }

    topology::IThread* getThread() override {
      auto thread = popThread();
      if(!thread){ 
        auto core = popCore();
        if(!core){
          auto tile = popTile(); 
          if(!tile){
            auto socket = popSocket();
            if(!socket){
              return nullptr;
            }
            tile = socket->getTile(0);
            for(size_t i = 1; i < socket->getNumTiles(); i++){
              pushTile(socket->getTile(i));
            }
          }
          ASSERT(tile);
          core = tile->getCore(0);
          for(size_t i = 1; i < tile->getNumCores(); i++){
            pushCore(tile->getCore(i));
          }
        }
        ASSERT(core);
        thread = core->getThread(0);
        for(size_t i = 1; i < core->getNumThreads(); i++){
          pushThread(core->getThread(i));
        }
      }
      return thread; 
    }

    topology::ISocket* sockets = nullptr;
    topology::ITile* tiles = nullptr;
    topology::ICore* cores = nullptr;
    topology::IThread* threads = nullptr;

    enum IdleState{
      DEEP_SLEEP,
      LOW_LATENCY_SLEEP,
      SPIN
    };

    IdleState idleState = SPIN;
  };

} // namespace mythos
