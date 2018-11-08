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
#include "mythos/protocol/PageMap.hh"
#include "runtime/Frame.hh"
#include "runtime/KernelMemory.hh"
#include "mythos/init.hh"

namespace mythos {

  class PageMap : public KObject
  {
  public:
    typedef protocol::PageMap::PageMapReq PageMapReq;
    typedef protocol::PageMap::MapFlags MapFlags;

    PageMap() {}
    PageMap(CapPtr cap) : KObject(cap) {}

    PortalFuture<void> create(PortalLock pr, KernelMemory kmem, size_t level,
                              CapPtr factory = init::PAGEMAP_FACTORY) {
      return pr.invoke<protocol::PageMap::Create>(kmem.cap(), _cap, factory, level);
    }

    struct Result {
      Result() {}
      Result(InvocationBuf* ib) {
        auto msg = ib->cast<protocol::PageMap::Result>();
        vaddr = msg->vaddr;
	size = msg->size;
        level = msg->level;
      }
      uintptr_t vaddr = 0;
      size_t size = 0;
      size_t level = 0;
    };

    PortalFuture<Result>
    installMap(PortalLock pr, PageMap pagemap, uintptr_t vaddr, size_t level, MapFlags flags) {
      return pr.invoke<protocol::PageMap::InstallMap>(_cap, pagemap.cap(), vaddr, level, flags);
    }

    PortalFuture<Result>
    removeMap(PortalLock pr, uintptr_t vaddr, size_t level) {
      return pr.invoke<protocol::PageMap::RemoveMap>(_cap, vaddr, level);
    }

    PortalFuture<Result>
    mmap(PortalLock pr, Frame frame, uintptr_t vaddr, size_t size, MapFlags flags, size_t offset=0) {
      return pr.invoke<protocol::PageMap::Mmap>(_cap, frame.cap(), vaddr, size, flags, offset);
    }

    PortalFuture<Result>
    remap(PortalLock pr, uintptr_t sourceAddr, uintptr_t destAddr, size_t size) {
      return pr.invoke<protocol::PageMap::Remap>(_cap, sourceAddr, destAddr, size);
    }

    PortalFuture<Result>
    mprotect(PortalLock pr, uintptr_t vaddr, size_t size, MapFlags flags) {
      return pr.invoke<protocol::PageMap::Mprotect>(_cap, vaddr, size, flags);
    }

    PortalFuture<Result>
    munmap(PortalLock pr, uintptr_t vaddr, size_t size) {
      return pr.invoke<protocol::PageMap::Munmap>(_cap, vaddr, size);
    }
  };

} // namespace mythos
