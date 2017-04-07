/* -*- mode:C++; indent-tabs-mode:nil; -*- */
/* MyThOS: The Many-Threads Operating System
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
 * Copyright 2016 Randolf Rotta, Robert Kuban, and contributors, BTU Cottbus-Senftenberg
 */
#pragma once

#include "runtime/PortalBase.hh"
#include "mythos/protocol/ExecutionContext.hh"
#include "runtime/CapMap.hh"
#include "runtime/PageMap.hh"
#include "runtime/UntypedMemory.hh"
#include "mythos/init.hh"

namespace mythos {

  class ExecutionContext : public KObject
  {
  public:
    typedef protocol::ExecutionContext::Amd64Registers register_t;
    typedef void* (*StartFun)(void*);

    ExecutionContext(CapPtr cap) : KObject(cap) {}

    PortalFuture<void> create(PortalLock pr, UntypedMemory kmem,
                              PageMap as, CapMap cs, CapPtr sched,
                              void* stack, StartFun start, void* userctx,
                              CapPtr factory = init::EXECUTION_CONTEXT_FACTORY);

    PortalFuture<void> configure(PortalLock pr, PageMap as, CapMap cs, CapPtr sched) {
      return pr.invoke<protocol::ExecutionContext::Configure>(_cap, as.cap(), cs.cap(), sched);
    }

    struct Registers : register_t {
      Registers() {}
      Registers(InvocationBuf* ib)
        : register_t(ib->cast<protocol::ExecutionContext::WriteRegisters>()->regs)
      {}
    };

    PortalFuture<Registers> readRegisters(PortalLock pr, bool suspend) {
      return pr.invoke<protocol::ExecutionContext::ReadRegisters>(_cap, suspend);
    }
    PortalFuture<void> writeRegisters(PortalLock pr, register_t const& regs, bool resume) {
      return pr.invoke<protocol::ExecutionContext::WriteRegisters>(_cap, resume, regs);
    }
    PortalFuture<void> setFSGS(PortalLock pr, uintptr_t fs, uintptr_t gs) {
      return pr.invoke<protocol::ExecutionContext::SetFSGS>(_cap, fs,gs);
    }
    PortalFuture<void> resume(PortalLock pr) {
      return pr.invoke<protocol::ExecutionContext::Resume>(_cap);
    }
    PortalFuture<void> suspend(PortalLock pr) {
      return pr.invoke<protocol::ExecutionContext::Suspend>(_cap);
    }

  protected:
    static void start(StartFun main, void* userctx);
  };

} // namespace mythos
