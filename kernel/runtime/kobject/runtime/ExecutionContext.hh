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

namespace mythos {

  class ExecutionContext : public KObject
  {
  public:
    typedef protocol::ExecutionContext::Amd64Registers register_t;
    typedef void* (*StartFun)(void*);

    ExecutionContext(CapPtr cap) : KObject(cap) {}

    PortalFutureRef<void> create(PortalRef pr, UntypedMemory kmem, CapPtr factory,
                                 PageMap as, CapMap cs, CapPtr sched,
                                 void* stack, StartFun start, void* userctx);

    PortalFutureRef<void> configure(PortalRef pr, PageMap as, CapMap cs, CapPtr sched);

    class RegsRef : public PortalFutureRefBase
    {
    public:
      RegsRef(PortalRef&& p) : PortalFutureRefBase(std::move(p)) {}

      /** quick access to the invocation result */
      register_t* operator-> () const {
        return &_portal->buf()->cast<protocol::ExecutionContext::WriteRegisters>()->regs;
      }
      register_t& operator* () const {
        return _portal->buf()->cast<protocol::ExecutionContext::WriteRegisters>()->regs;
      }
    };

    RegsRef readRegisters(PortalRef pr, bool suspend);
    PortalFutureRef<void> writeRegisters(PortalRef pr, register_t regs, bool resume);
    PortalFutureRef<void> setFSGS(PortalRef pr, uintptr_t fs, uintptr_t gs);
    PortalFutureRef<void> resume(PortalRef pr);
    PortalFutureRef<void> suspend(PortalRef pr);

  protected:
    static void start(StartFun main, void* userctx);
  };

} // namespace mythos
