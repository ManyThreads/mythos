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
#include "mythos/protocol/Frame.hh"
#include "runtime/KernelMemory.hh"
#include "mythos/init.hh"

namespace mythos {

  class Frame : public KObject
  {
  public:
    Frame() {}
    Frame(CapPtr cap) : KObject(cap) {}

    PortalFuture<void> create(PortalLock pr, KernelMemory kmem,
                              size_t size, size_t alignment,
                              CapPtr factory = init::MEMORY_REGION_FACTORY) {
      return pr.invoke<protocol::Frame::Create>(kmem.cap(), _cap, factory, size, alignment);
    }

    PortalFuture<void> createDevice(PortalLock pr, KObject root,
                              size_t addr, size_t size, bool writable) {
      return pr.invoke<protocol::MemoryRoot::Create>(root.cap(), _cap, addr, size, writable);
    }

    struct Info {
      Info() {}
      Info(InvocationBuf* ib) {
        auto msg = ib->cast<protocol::Frame::Info>();
        addr = msg->addr;
        size = msg->size;
        writable = msg->writable;
      }
      uint64_t addr = 0;
      uint64_t size = 0;
      bool writable = false;
    };

    PortalFuture<Info> info(PortalLock pr) {
      return pr.invoke<protocol::Frame::Info>(_cap);
    }
  };

} // namespace mythos
