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
#include "objects/DeviceMemory.hh"
#include "boot/mlog.hh"

namespace mythos {
  namespace boot {
    // be careful with pointers to these objects because they are image addresses
    DeviceMemory _device_memory_root;
    KernelMemory _kmem_root(nullptr, Range<uintptr_t>::bySize(KERNELMEM_ADDR, KERNELMEM_SIZE));
    CapEntry _kmem_root_entry;

    DeviceMemory* device_memory_root() { return image2kernel(&_device_memory_root); }
    CapEntry& device_memory_root_entry () { return device_memory_root()->get_cap_entry(); }
    KernelMemory* kmem_root() { return image2kernel(&_kmem_root); }
    CapEntry* kmem_root_entry() { return image2kernel(&_kmem_root_entry); }

    void initMemoryRegions() {
      MLOG_INFO(mlog::boot, "initialise memory regions");
      device_memory_root()->init();
      kmem_root_entry()->acquire();
      cap::inherit(device_memory_root_entry(), device_memory_root_entry().cap(), 
                   *kmem_root_entry(), Cap(kmem_root()));
    }
  } // namespace boot
} // namespace mythos
