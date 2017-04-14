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
#include "mythos/protocol/CapMap.hh"
#include "runtime/UntypedMemory.hh"
#include "mythos/init.hh"

namespace mythos {

  class CapMap : public KObject
  {
  public:
    CapMap(CapPtr cap) : KObject(cap) {}

    PortalFuture<void> create(PortalLock pr, UntypedMemory kmem,
                              CapPtrDepth indexbits, CapPtrDepth guardbits, CapPtr guard,
                              CapPtr factory = init::CAPMAP_FACTORY) {
      return pr.invoke<protocol::CapMap::Create>(kmem.cap(), _cap, factory,
                                               indexbits, guardbits, guard);
    }

    PortalFuture<void> derive(PortalLock pr, CapPtr src, CapPtrDepth srcDepth,
                                 CapPtr dstCs, CapPtr dst, CapPtrDepth dstDepth,
                                 CapRequest req) {
      return pr.invoke<protocol::CapMap::Derive>(_cap, src, srcDepth, dstCs, dst, dstDepth, req);
    }

    PortalFuture<void> reference(PortalLock pr, CapPtr src, CapPtrDepth srcDepth,
                                    CapPtr dstCs, CapPtr dst, CapPtrDepth dstDepth,
                                    CapRequest req) {
      return pr.invoke<protocol::CapMap::Reference>(_cap, src, srcDepth, dstCs, dst, dstDepth, req);
    }

    PortalFuture<void> move(PortalLock pr, CapPtr src, CapPtrDepth srcDepth,
                                 CapPtr dstCs, CapPtr dst, CapPtrDepth dstDepth) {
      return pr.invoke<protocol::CapMap::Move>(_cap, src, srcDepth, dstCs, dst, dstDepth);
    }

    PortalFuture<void> deleteCap(PortalLock pr, CapPtr src, CapPtrDepth srcDepth) {
      return pr.invoke<protocol::CapMap::Delete>(_cap, src, srcDepth);
    }

    PortalFuture<void> deleteCap(PortalLock pr, KObject src) {
      return deleteCap(pr, src.cap(), mythos::max_cap_depth);
    }

    PortalFuture<void> revokeCap(PortalLock pr, CapPtr src, CapPtrDepth srcDepth) {
      return pr.invoke<protocol::CapMap::Revoke>(_cap, src, srcDepth);
    }

    PortalFuture<void> revokeCap(PortalLock pr, KObject src) {
      return revokeCap(pr, src.cap(), mythos::max_cap_depth);
    }
  };

} // namespace mythos
