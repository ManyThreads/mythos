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

#define PS_PER_TSC_DEFAULT (0x181)

namespace mythos {

class ProcessorManagerInfo{
  public:
    //ProcessorManagerInfo(){}

    virtual void setIdle(size_t id) = 0;
    virtual void setRunning(size_t id) = 0;
    
};

template<size_t NUMTHREADS>
class FixedSizedProcessorManagerInfo 
  : public ProcessorManagerInfo
{ 
  public:
    FixedSizedProcessorManagerInfo(){
      for(int i = 0; i < NUMTHREADS; i++){
        states[i] = UNALLOCATED;
      }
    }

    enum ProzessorState{
      INVALID,
      UNALLOCATED,
      IDLE,
      RUNNING     
    };

    void setIdle(size_t id){
      states[id].store(IDLE);
    }

    void setRunning(size_t id){
      states[id].store(RUNNING);
    }
    
    optional<size_t> reuseIdle(){
      for(size_t i = 0; i < NUMTHREADS; i++){
        if(states[i].compare_exchange_weak(IDLE, RUNNING)) return i;
      }
      return optional<size_t>();
    }
  private:
    std::atomic<ProzessorState> states[NUMTHREADS];
};

class ProcessInfoFrame{
  public:
    ProcessInfoFrame()
      : psPerTsc(PS_PER_TSC_DEFAULT)
    {}

    uint64_t getPsPerTSC() { return psPerTsc; }
    InvocationBuf* getInvocationBuf() {return &ib; }
    virtual size_t getNumThreads() = 0;
    virtual ProcessorManagerInfo* getProcessorManagerInfo() = 0;
    virtual uintptr_t getInfoEnd () = 0;

  //protected: //todo manage access
    //friend class boot::Initloader;
    InvocationBuf ib; // needs to be the first member (see Initloader::createPortal)
    uint64_t psPerTsc; // picoseconds per time stamp counter
};
template<size_t NUMTHREADS>
class FixedSizedProcessInfoFrame
  : public ProcessInfoFrame
{
  public:
    FixedSizedProcessInfoFrame(){}

    size_t getNumThreads() override { return NUMTHREADS; }
    ProcessorManagerInfo* getProcessorManagerInfo() override { return &pmi; }
    uintptr_t getInfoEnd () override { return reinterpret_cast<uintptr_t>(this) + sizeof(ProcessInfoFrame); }

  //protected: //todo manage access
    //friend class boot::Initloader;
    FixedSizedProcessorManagerInfo<NUMTHREADS> pmi;  
};

}// namespace mythos
