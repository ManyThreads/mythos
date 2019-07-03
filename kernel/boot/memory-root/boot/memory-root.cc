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

#include "boot/memory-root.hh"

#include "objects/CapEntry.hh"
#include "objects/ops.hh"
#include "objects/KernelMemory.hh"
#include "objects/StaticMemoryRegion.hh"
#include "objects/MemoryRoot.hh"
#include "boot/mlog.hh"

namespace mythos {
  namespace boot {
    // be careful with pointers to these objects because they are image addresses
    MemoryRoot _cap_root;
    StaticMemoryRegion _memory_region[STATIC_MEMORY_REGIONS];
    KernelMemory _kmem_root(nullptr, Range<uintptr_t>::bySize(KERNELMEM_ADDR, KERNELMEM_SIZE));
    CapEntry _kmem_root_entry;

    MemoryRoot* cap_root() { return image2kernel(&_cap_root); }

    StaticMemoryRegion* memory_region(size_t index) {
      ASSERT(index < STATIC_MEMORY_REGIONS);
      return image2kernel(&_memory_region[index]);
    }

    CapEntry* kmem_root_entry() { return image2kernel(&_kmem_root_entry); }
    KernelMemory* kmem_root() { return image2kernel(&_kmem_root); }

    void initMemoryRegions() {
      MLOG_INFO(mlog::boot, "initialise memory regions");
      cap_root()->init();
      for (size_t i = 0; i < STATIC_MEMORY_REGIONS; ++i) {
        auto res = memory_region(i)->init(i, cap_root()->getRoot());
        ASSERT(res);
      }

      kmem_root_entry()->acquire();
      cap::inherit(memory_region(0)->getRoot(), memory_region(0)->getRoot().cap(), 
                   *kmem_root_entry(), Cap(kmem_root()));
    }
    
  } // namespace boot
} // namespace mythos
