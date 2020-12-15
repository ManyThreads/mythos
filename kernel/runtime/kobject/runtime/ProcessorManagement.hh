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

#include "runtime/PortalBase.hh"
#include "mythos/protocol/ProcessorManagement.hh"
#include "mythos/init.hh"
#include "mythos/ProcessInfoFrame.hh"

extern mythos::ProcessInfoFrame* info_ptr asm("info_ptr");

namespace mythos {

  class ProcessorManagement : public KObject
  {
  public:

    ProcessorManagement() 
      : pmi(info_ptr->getProcessorManagerInfo())
    {}

    ProcessorManagement(CapPtr cap) 
      : KObject(cap) 
      , pmi(info_ptr->getProcessorManagerInfo())
    {}

    struct AllocCoreResult{
      AllocCoreResult() {}
      AllocCoreResult(InvocationBuf* ib) {
        auto msg = ib->cast<protocol::ProcessorManagement::RetAllocCore>();
        sc = msg->sc();
      }
     
      CapPtr sc = null_cap;
    };

    PortalFuture<AllocCoreResult> allocCore(PortalLock pr){
      //auto reuse = pmi->reuseIdle();
      //if(reuse){
        //return (init::SCHEDULERS_START + *reuse)
      //}
      return pr.invoke<protocol::ProcessorManagement::AllocCore>(_cap);
      
    }

    //PortalFuture<void> freeCore(PortalLock pr, CapPtr sc){
      //return pr.invoke<protocol::ProcessorManagement::FreeCore>(_cap, sc);
    //}

    //void markFree(CapPtr cs){
      //pmi->setIdle(sc - init::SCHEDULERS_START);
    //}

  private:
    ProcessorManagerInfo* pmi;

  };

} // namespace mythos
