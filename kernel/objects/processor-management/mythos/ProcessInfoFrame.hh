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

//#include "cpu/hwthreadid.hh"
#include "mythos/InvocationBuf.hh"
#include "util/align.hh"

#define PS_PER_TSC_DEFAULT (0x180)

namespace mythos {

class ProcessorManagerInfo 
{ 
  public:
    enum ProcessorState{
      INVALID,
      FREE, //unallocated
      IDLE,
      RUNNING     
    };

    ProcessorManagerInfo(size_t numThreads)
      : numThreads(numThreads)
      , states(reinterpret_cast<std::atomic<ProcessorState>* >(round_up(reinterpret_cast<uintptr_t>(this) + sizeof(this), alignLine)))
    {
      states = new(states) std::atomic<ProcessorState>[numThreads]();
      for(size_t i = 0; i < numThreads; i++) states[i].store(FREE);
    }

    void setIdle(size_t id){
      states[id].store(IDLE);
    }

    void setRunning(size_t id){
      states[id].store(RUNNING);
    }
    
    void setFree(size_t id){
      states[id].store(FREE);
    }

    optional<size_t> reuseIdle(){
      for(size_t i = 0; i < numThreads; i++){
        auto state = IDLE;
        if(states[i].compare_exchange_weak(state, RUNNING)){
          return i;
        }
      }
      return optional<size_t>();
    }

    size_t getNumThreads() { return numThreads; } 
    static size_t getThreadArraySize(size_t numThreads){ return sizeof(std::atomic<ProcessorState>) * numThreads + alignLine; }
  private:
    size_t numThreads;
    std::atomic<ProcessorState> *states;
};

class ProcessInfoFrame{
  public:
    ProcessInfoFrame(size_t numThreads)
      : psPerTsc(PS_PER_TSC_DEFAULT)
      , pmi(numThreads)
    {}

    uint64_t getPsPerTSC() { return psPerTsc; }
    InvocationBuf* getInvocationBuf() {return &ib; }
    size_t getNumThreads() { return pmi.getNumThreads(); }
    ProcessorManagerInfo* getProcessorManagerInfo() { return &pmi; }
    uintptr_t getInfoEnd () { return reinterpret_cast<uintptr_t>(this) + sizeof(ProcessInfoFrame); }

    static size_t getInfoSize(size_t numThreads){ return sizeof(ProcessInfoFrame) + ProcessorManagerInfo::getThreadArraySize(numThreads); }

  //protected: //todo manage access
    //friend class boot::Initloader;
    InvocationBuf ib; // needs to be the first member (see Initloader::createPortal)
    uint64_t psPerTsc; // picoseconds per time stamp counter
    ProcessorManagerInfo pmi; // needs to be last member, because the HW Thread-State Array is following
    // ATTENTION: ProcessorManagementInfo creates an array for HW-Threads behind it's own object (please use getInfoEnd/getInfoSize function)
};

}// namespace mythos
