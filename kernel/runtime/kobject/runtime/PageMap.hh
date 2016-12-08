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
#include "mythos/protocol/PageMap.hh"
#include "runtime/Frame.hh"
#include "runtime/UntypedMemory.hh"

namespace mythos {

  class PageMap : public KObject
  {
  public:
    typedef protocol::PageMap::PageMapReq PageMapReq;
    typedef protocol::PageMap::MapFlags MapFlags;

    PageMap(CapPtr cap) : KObject(cap) {}

    PortalFutureRef<void> create(PortalRef pr, UntypedMemory kmem, CapPtr factory,
                                 size_t level) {
      return pr.tryInvoke<protocol::PageMap::Create>(kmem.cap(), _cap, factory, level);
    }

    PortalFutureRef<protocol::PageMap::Result>
    installMap(PortalRef pr, PageMap pagemap, uintptr_t vaddr, size_t level, MapFlags flags) {
      return pr.tryInvoke<protocol::PageMap::InstallMap>(_cap, pagemap.cap(), vaddr, level, flags);
    }

    PortalFutureRef<protocol::PageMap::Result>
    removeMap(PortalRef pr, uintptr_t vaddr, size_t level) {
      return pr.tryInvoke<protocol::PageMap::RemoveMap>(_cap, vaddr, level);
    }

    PortalFutureRef<protocol::PageMap::Result>
    mmap(PortalRef pr, Frame frame, uintptr_t vaddr, size_t size, MapFlags flags) {
      return pr.tryInvoke<protocol::PageMap::Mmap>(_cap, frame.cap(), vaddr, size, flags);
    }

    PortalFutureRef<protocol::PageMap::Result>
    remap(PortalRef pr, uintptr_t sourceAddr, uintptr_t destAddr, size_t size) {
      return pr.tryInvoke<protocol::PageMap::Remap>(_cap, sourceAddr, destAddr, size);
    }

    PortalFutureRef<protocol::PageMap::Result>
    mprotect(PortalRef pr, uintptr_t vaddr, size_t size, MapFlags flags) {
      return pr.tryInvoke<protocol::PageMap::Mprotect>(_cap, vaddr, size, flags);
    }

    PortalFutureRef<protocol::PageMap::Result>
    munmap(PortalRef pr, uintptr_t vaddr, size_t size) {
      return pr.tryInvoke<protocol::PageMap::Munmap>(_cap, vaddr, size);
    }
  };

} // namespace mythos
