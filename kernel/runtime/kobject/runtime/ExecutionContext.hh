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
 * Copyright 2016 Randolf Rotta, Robert Kuban, and contributors, BTU Cottbus-Senftenberg
 */
#pragma once

#include "runtime/PortalBase.hh"
#include "mythos/protocol/ExecutionContext.hh"
#include "runtime/CapMap.hh"
#include "runtime/PageMap.hh"
#include "runtime/KernelMemory.hh"
#include "mythos/init.hh"

namespace mythos {

    extern thread_local CapPtr localEC;

    template<class MSG> class Msg;

    template<> class Msg<protocol::ExecutionContext::Create>
    {
    protected:
        protocol::ExecutionContext::Create ib;
        CapPtr obj;
    public:
        typedef void* (*StartFun)(void*);
        typedef int (*StartFunInt)(void*);

        Msg(CapPtr dst, CapPtr kmem, CapPtr factory) : ib(dst, factory), obj(kmem) {}

        Msg& as(CapPtr c) { ib.as(c); return *this; }
        Msg& as(PageMap& c) { ib.as(c.cap()); return *this; }
        Msg& cs(CapPtr c) { ib.cs(c); return *this; }
        Msg& cs(CapMap& c) { ib.cs(c.cap()); return *this; }
        Msg& sched(CapPtr c) { ib.sched(c); return *this; }
        Msg& state(CapPtr c, uint32_t offset) { ib.state(c); ib.stateOffset = offset; return *this; }
        Msg& rawStack(void* ptr) { ib.regs.rsp = uintptr_t(ptr); return *this; }
        Msg& prepareStack(void* ptr) {
            // \TODO assert alignment
            auto tos = reinterpret_cast<uintptr_t*>(ptr);
            *--tos = 0;                                     // dummy return address
            ib.regs.rsp = uintptr_t(tos);
            return *this;
        }
        Msg& startFun(StartFun fun, void* userctx, CapPtr ec) {
            ib.regs.rdi = uintptr_t(fun);                   // arg 1
            ib.regs.rsi = uintptr_t(userctx);               // arg 2
            ib.regs.rdx = uintptr_t(ec);                    // arg 3
            ib.regs.rip = uintptr_t(&start);
            return *this;
        }

        Msg& startFunInt(StartFunInt fun, void* userctx, CapPtr ec) {
            ib.regs.rdi = uintptr_t(fun);                   // arg 1
            ib.regs.rsi = uintptr_t(userctx);               // arg 2
            ib.regs.rdx = uintptr_t(ec);                    // arg 3
            ib.regs.rip = uintptr_t(&startInt);
            return *this;
        }

        Msg& suspended(bool value) { ib.start = !value; return *this; }
        Msg& fs(void* ptr) { ib.regs.fs_base = uintptr_t(ptr); return *this; }
        Msg& gs(void* ptr) { ib.regs.gs_base = uintptr_t(ptr); return *this; }

        // inherited from KernelMemory::CreateBase
        Msg& indirectDest(CapPtr dstCSpace, CapPtrDepth dstDepth) {
            ib.setIndirectDest(dstCSpace, dstDepth);
            return *this;
        }

        PortalFuture<void> invokeVia(PortalLock pl) {

            return std::move(pl.invokeWithMsg(obj,  ib));
        }

    protected:
        static void start(StartFun main, void* userctx, CapPtr ec) {
            localEC = ec;
            syscall_exit(uintptr_t(main(userctx)));
        }

        static void startInt(StartFunInt main, void* userctx, CapPtr ec) {
            localEC = ec;
            syscall_exit(main(userctx));
        }
    };


  class ExecutionContext : public KObject
  {
  public:
    typedef protocol::ExecutionContext::Amd64Registers register_t;
    typedef int (*StartFun)(void*);

    ExecutionContext() {}
    ExecutionContext(CapPtr cap) : KObject(cap) {}

    Msg<protocol::ExecutionContext::Create>
    create(KernelMemory kmem, CapPtr factory = init::EXECUTION_CONTEXT_FACTORY) {
        return {this->cap(), kmem.cap(), factory};
    }

    PortalFuture<void> configure(PortalLock pr, PageMap as, CapMap cs, CapPtr sched, CapPtr state, uint32_t stateOffset, bool initializeState) {
      return pr.invoke<protocol::ExecutionContext::Configure>(_cap, as.cap(), cs.cap(), sched, state, stateOffset, initializeState);
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

  };

} // namespace mythos
