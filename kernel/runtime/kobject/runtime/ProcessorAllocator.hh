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
#include "mythos/protocol/ProcessorAllocator.hh"
#include "mythos/init.hh"

namespace mythos {

  class ProcessorAllocator : public KObject
  {
  public:

    ProcessorAllocator() {}
    ProcessorAllocator(CapPtr cap) : KObject(cap) {}

    struct AllocResult{
      AllocResult() {}
      AllocResult(InvocationBuf* ib) {
        auto msg = ib->cast<protocol::ProcessorAllocator::RetAlloc>();
        cap = msg->sc();
      }
     
      CapPtr cap = null_cap;
    };

    PortalFuture<AllocResult> alloc(PortalLock pr){
      return pr.invoke<protocol::ProcessorAllocator::Alloc>(_cap);
    }

    PortalFuture<AllocResult> alloc(PortalLock pr, CapPtr dstMap){
      return pr.invoke<protocol::ProcessorAllocator::Alloc>(_cap, dstMap);
    }

    PortalFuture<void> free(PortalLock pr, CapPtr sc){
      return pr.invoke<protocol::ProcessorAllocator::Free>(_cap, sc);
    }

  };

} // namespace mythos
