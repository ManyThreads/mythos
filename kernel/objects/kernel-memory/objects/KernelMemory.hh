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

#include <array>
#include "util/assert.hh"
#include "async/NestedMonitorDelegating.hh"
#include "objects/FirstFitHeap.hh"
#include "objects/IAllocator.hh"
#include "objects/IKernelObject.hh"
#include "objects/CapEntry.hh"
#include "objects/ops.hh"
#include "objects/IFactory.hh"
#include "mythos/protocol/KernelMemory.hh"

#include "async/mlog.hh"
#include "objects/mlog.hh"

namespace mythos {

  class KernelMemory final
    : public IKernelObject
    , public IAllocator
  {
  public:
    /** manages free memory in the range. The range is given in logical
     * addresses of kernel memory and internally converted to the
     * physical range. */
    KernelMemory(IAsyncFree* parent, Range<uintptr_t> range);
    KernelMemory(const KernelMemory&) = delete;
    virtual ~KernelMemory() {}

    /** called by factories and boot::initKernelMemory to add free memory */
    void addRange(PhysPtr<void> start, size_t length);

  public: // IAllocator interface: synchrounous methods for initialisation and internal use
    optional<void*> alloc(size_t length, size_t alignment) override;
    optional<void> alloc(MemoryDescriptor* begin, MemoryDescriptor* end) override;
    void free(void* ptr, size_t length) override;
    void free(MemoryDescriptor* begin, MemoryDescriptor* end) override;

  public: // IAsyncFree interface
    void free(Tasklet* t, IResult<void>* r,
              MemoryDescriptor* begin, MemoryDescriptor* end) override;
    void free(Tasklet* t, IResult<void>* r, void* start, size_t length) override;

  public: // IKernelObject interface
    optional<void const*> vcast(TypeId id) const override {
      if (id == typeId<IAllocator>()) return static_cast<IAllocator const*>(this);
      THROW(Error::TYPE_MISMATCH);
    }

    Range<uintptr_t> addressRange(Cap) override { return _range; }

    optional<Cap> mint(Cap self, CapRequest, bool derive) override {
      if (derive) THROW(Error::INVALID_REQUEST);
      return self;
    }

    optional<void> deleteCap(Cap self, IDeleter& del) override {
      if (self.isOriginal()) del.deleteObject(del_handle);
      RETURN(Error::SUCCESS);
    }

    /** initiates the asynchronous final deletion of the memory area.
     * By design all objects that were allocated in this memory have
     * been deleted previously and returned their memory to this pool.
     */
    void deleteObject(Tasklet* t, IResult<void>* r) override {
      PANIC_MSG(_parent, "Attempted to delete root untyped memory!");
      monitor.doDelete(t, [=](Tasklet* t) {
          _parent->free(t, r, _memory.begin(), _memory.end());
        });
    }

    void invoke(Tasklet*, Cap, IInvocation* msg) override;
    Error invokeCreate(Tasklet* t, Cap self, IInvocation* msg);

  private:
    LinkedList<IKernelObject*>::Queueable del_handle = {this};
    IAsyncFree* _parent;
    FirstFitHeap<uintptr_t> heap;
    Range<uintptr_t> _range; //< for rangeProvided()
    /** for deletion: the managed range and then the own object */
    std::array<MemoryDescriptor, 2> _memory;
    async::NestedMonitorDelegating monitor;
  };

class KernelMemoryFactory : public FactoryBase
{
public:
  static optional<KernelMemory*>
  factory(CapEntry* dstEntry, CapEntry* memEntry, Cap memCap, IAllocator* mem,
          size_t size, size_t alignment);

  Error factory(CapEntry* dstEntry, CapEntry* memEntry, Cap memCap,
                IAllocator* mem, IInvocation* msg) const override {
    auto data = msg->getMessage()->cast<protocol::KernelMemory::Create>();
    return factory(dstEntry, memEntry, memCap, mem, data->size, data->alignment).state();
  }
};

} // namespace mythos
