/* -*- mode:C++; -*- */
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
#include "objects/MemoryRegion.hh"
#include "util/alignments.hh"

namespace mythos {

  MemoryRegion::MemoryRegion(IAsyncFree* mem, PhysPtr<void> start, size_t size)
    : size(size), _mem(mem)
  {
    ASSERT(isKernelAddress(this));
    frame.start = start.physint();
    _memdesc[0] = MemoryDescriptor(start.log(), size);
    _memdesc[1] = MemoryDescriptor(this, sizeof(MemoryRegion));
  }

  optional<MemoryRegion*>
  MemoryRegionFactory::factory(CapEntry* dstEntry, CapEntry* memEntry, Cap memCap, IAllocator* mem,
			      size_t size, size_t alignment)
  {
    if (!Align4k::is_aligned(alignment) || !AlignmentObject(alignment).is_aligned(size))
      THROW(Error::UNALIGNED);
    auto region = mem->alloc(size, alignment);
    if (!region) RETHROW(region);
    auto obj = mem->create<MemoryRegion>(PhysPtr<void>::fromKernel(*region), size);
    if (!obj) {
      mem->free(*region, size);
      RETHROW(obj);
    }
    auto res = cap::inherit(*memEntry, *dstEntry, memCap, Cap(*obj, FrameData()));
    if (!res) {
      mem->free(*obj); // mem->release(obj) goes throug IKernelObject deletion mechanism
      mem->free(*region, size);
      RETHROW(res);
    }
    return *obj;
  }

} // namespace mythos
