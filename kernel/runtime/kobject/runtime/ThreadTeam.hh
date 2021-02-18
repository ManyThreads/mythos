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

#include "runtime/PortalBase.hh"
#include "mythos/protocol/ThreadTeam.hh"
#include "runtime/ExecutionContext.hh"
#include "mythos/init.hh"

namespace mythos {

  class ThreadTeam : public KObject
  {
  public:

    ThreadTeam() {}
    ThreadTeam(CapPtr cap) : KObject(cap) {}

    PortalFuture<void> create(PortalLock pr, KernelMemory kmem, CapPtr pa, CapPtr factory = init::THREADTEAM_FACTORY){
      return pr.invoke<protocol::ThreadTeam::Create>(kmem.cap(), _cap, factory, pa);
    }

    struct RunResult{
      RunResult() {}
      RunResult(InvocationBuf* ib) {
        auto msg = ib->cast<protocol::ThreadTeam::RetTryRunEC>();
        response = msg->response;
      }

      bool failed() const { return response == protocol::ThreadTeam::RetTryRunEC::FAILED; }
      bool allocated() const { return response == protocol::ThreadTeam::RetTryRunEC::ALLOCATED; }
      bool notFailed() const { return response == protocol::ThreadTeam::RetTryRunEC::ALLOCATED 
        || response == protocol::ThreadTeam::RetTryRunEC::DEMANDED 
        || response == protocol::ThreadTeam::RetTryRunEC::FORCED; } 
      int response = 0;
    };

    PortalFuture<RunResult> tryRunEC(PortalLock pr, ExecutionContext ec, int at = protocol::ThreadTeam::FAIL){
      return pr.invoke<protocol::ThreadTeam::TryRunEC>(_cap, ec.cap(), at);
    }

    PortalFuture<void> runNextToEC(PortalLock pr, ExecutionContext ec, ExecutionContext ec_place){
      return pr.invoke<protocol::ThreadTeam::RunNextToEC>(_cap, ec.cap(), ec_place.cap());
    }

    struct RevokeResult{
      RevokeResult() {}
      RevokeResult(InvocationBuf* ib) {
        auto msg = ib->cast<protocol::ThreadTeam::RetRevokeDemand>();
        revoked = msg->revoked;
      }

      bool revoked = false;
    };
    PortalFuture<RevokeResult> revokeDemand(PortalLock pr, ExecutionContext ec){
      return pr.invoke<protocol::ThreadTeam::RevokeDemand>(_cap, ec.cap());
    }
  };

} // namespace mythos
