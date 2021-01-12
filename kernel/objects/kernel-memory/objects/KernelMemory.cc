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

#include "objects/KernelMemory.hh"
#include "objects/TypedCap.hh"

#include <new>
#include "util/assert.hh"
#include "util/align.hh"

namespace mythos {

KernelMemory::KernelMemory(IAsyncFree* parent, Range<uintptr_t> range)
  : _parent(parent)
{
  _memory[0].ptr = reinterpret_cast<void*>(range.getStart());
  _memory[0].size = range.getSize();
  _memory[1].ptr = this;
  _memory[1].size = sizeof(KernelMemory);
  _range = Range<uintptr_t>::bySize(
      PhysPtr<void>::fromKernel(_memory[0].ptr).physint(),
      range.getSize());
}

optional<void*> KernelMemory::alloc(size_t length, size_t alignment)
{
  auto result = heap.alloc(length, alignment);
  if (result) {
    MLOG_INFO(mlog::km, "alloc", DVAR(length), DVARhex(alignment), DVARhex(*result));
    monitor.acquireRef();
    return reinterpret_cast<void*>(*result);
  } else {
    MLOG_INFO(mlog::km, "alloc failed", DVAR(length), DVARhex(alignment), DVAR(result.state()));
    RETHROW(result);
  }
}

optional<void> KernelMemory::alloc(MemoryDescriptor* begin, MemoryDescriptor* end)
{
  auto result = optional<uintptr_t>(1);
  auto it = begin;
  for (; it != end; ++it) {
    ASSERT(it->size > 0);
    ASSERT(!it->ptr);
    result = heap.alloc(it->size, it->alignment);
    if (result) {
      MLOG_INFO(mlog::km, "alloc", DVAR(it->size), DVARhex(it->alignment), DVARhex(*result));
      monitor.acquireRef();
      it->ptr = reinterpret_cast<void*>(*result);
    } else {
      MLOG_INFO(mlog::km, "alloc failed", DVAR(it->size), DVARhex(it->alignment), DVAR(result.state()));
      break;
    }
  }
  if (!result) {
    free(begin, it);
    RETHROW(result);
  } else {
    RETURN(Error::SUCCESS);
  }
}

void KernelMemory::free(void* ptr, size_t length)
{
  auto start = reinterpret_cast<uintptr_t>(ptr);
  auto freeRange = Range<uintptr_t>::bySize(PhysPtr<void>::fromKernel(ptr).physint(), length);
  MLOG_INFO(mlog::km, "free", DVARhex(ptr), DVARhex(length));
  ASSERT(_range.contains(freeRange));
  monitor.releaseRef();
  heap.free(start, length);
}

void KernelMemory::free(MemoryDescriptor* begin, MemoryDescriptor* end)
{
  for (auto it = begin; it != end; ++it) free(it->ptr, it->size);
}

void KernelMemory::addRange(PhysPtr<void> start, size_t length)
{
  MLOG_INFO(mlog::km, "addRange: chunk = ", DVARhex(start.physint()), DVAR(length));
  // if not in kernel memory address range put it to user mem 
  if (!start.kernelmem()){
    MLOG_INFO(mlog::km, "addRange: completely to user mem (physical)", DVARhex(start.physint()), DVAR(length));
    Range<uintptr_t> umem = Range<uintptr_t>::bySize(start.physint(), length);
    _umem.add(umem); 
  }
  // if it begins in kernel memory range split range according to kernel memory limit
  else{
    //logical address range for kmem
    Range<uintptr_t> kmem = Range<uintptr_t>::bySize(start.logint(), length);
    Range<uintptr_t> krange =
      Range<uintptr_t>::bySize(PhysPtr<void>(_range.getStart()).logint(), _range.getSize());
    kmem = kmem.cut(krange);
    if (!kmem.isEmpty()){
      MLOG_INFO(mlog::km, "addRange: to kernel mem (logical)", DVARhex(kmem.getStart()), DVAR(kmem.getSize()));
      heap.addRange(kmem.getStart(), kmem.getSize());
    }

    //physical address range for umem
    Range<uintptr_t> umem = Range<uintptr_t>::bySize(start.physint(), length);
    Range<uintptr_t> urange =
      Range<uintptr_t>::bySize(_range.getStart() + _range.getSize(), length);
    umem = umem.cut(urange);
    if (!umem.isEmpty()){
      MLOG_INFO(mlog::km, "addRange: to user user mem (physical)", DVARhex(umem.getStart()), DVAR(umem.getSize()));
      _umem.add(umem);
    } 
  }
}


// IAsyncFree interface

void KernelMemory::free(Tasklet* t, IResult<void>* r,
       MemoryDescriptor* begin, MemoryDescriptor* end)
{
  MLOG_INFO(mlog::km, "free request", DVAR(t), DVAR(r), DVARhex(begin), DVARhex(end));
  monitor.request(t, [=](Tasklet* t){
      this->free(begin, end);
      r->response(t);
      this->monitor.requestDone();
    });
}

void KernelMemory::free(Tasklet* t, IResult<void>* r, void* start, size_t length)
{
  MLOG_INFO(mlog::km, "free request", DVAR(t), DVAR(r), DVARhex(start), DVARhex(length));
  monitor.request(t, [=](Tasklet* t){
      this->free(start, length);
      r->response(t);
      this->monitor.requestDone();
    });
}

  void KernelMemory::invoke(Tasklet* t, Cap self, IInvocation* msg)
  {
    monitor.request(t, [=](Tasklet* t){
        Error err = Error::NOT_IMPLEMENTED;
        switch (msg->getProtocol()) {
        // case protocol::KernelObject::proto:
        //   err = protocol::KernelObject::dispatchRequest(this, msg->getMethod(), t, self, msg);
        //   break;
        case protocol::KernelMemory::proto:
          err = protocol::KernelMemory::dispatchRequest(this, msg->getMethod(), t, self, msg);
          break;
        }
        if (err != Error::INHIBIT) {
          msg->replyResponse(err);
          monitor.requestDone();
        }
      } );
  }

  /// @todo return optional<void> instead of Error and use RETHROW instead of return x.state()
  Error KernelMemory::invokeCreate(Tasklet*, Cap self, IInvocation* msg)
  {
    auto ib = msg->getMessage();
    auto data = ib->read<protocol::KernelMemory::CreateBase>();

    MLOG_DETAIL(mlog::km, "create", DVAR(data.dstSpace()), DVAR(data.dstPtr));
    optional<CapEntry*> dstEntry;
    if (data.dstSpace() == null_cap) { // direct address
      dstEntry = msg->lookupEntry(data.dstPtr, 32, true); // lookup for write access
      if (!dstEntry) return dstEntry.state();
    } else { // indirect address
      TypedCap<ICapMap> dstSpace(msg->lookupEntry(data.dstSpace()));
      if (!dstSpace) return dstSpace.state();
      auto dstEntryRef = dstSpace.lookup(data.dstPtr, data.dstDepth, true); // lookup for write
      if (!dstEntryRef) return dstEntryRef.state();
      dstEntry = dstEntryRef->entry;
    }

    MLOG_DETAIL(mlog::km, "create", DVAR(*dstEntry), DVAR(data.factory()));
    TypedCap<IFactory> factory(msg->lookupEntry(data.factory()));
    if (!factory) return factory;

    MLOG_DETAIL(mlog::km, "create", DVAR(*factory));
    if (!dstEntry->acquire()) return Error::LOST_RACE;
    auto res = factory->factory(*dstEntry, msg->getCapEntry(), self, this, msg);
    return res;
  }

  optional<KernelMemory*>
  KernelMemoryFactory::factory(CapEntry* dstEntry, CapEntry* memEntry, Cap memCap, IAllocator* mem,
                               size_t size, size_t alignment)
  {
    if (!is_aligned(alignment, align4K) || !is_aligned(size,alignment))
      THROW(Error::UNALIGNED);
    auto region = mem->alloc(size, alignment);
    if (!region) {
      dstEntry->reset();
      RETHROW(region);
    }
    auto obj =
      mem->create<KernelMemory>(Range<uintptr_t>(uintptr_t(*region), uintptr_t(*region)+size));
    if (!obj) {
      mem->free(*region, size);
      dstEntry->reset();
      RETHROW(obj);
    }
    Cap cap(*obj);
    auto res = cap::inherit(*memEntry, memCap, *dstEntry, cap);
    if (!res) {
      mem->free(*obj); // mem->release(obj) goes throug IKernelObject deletion mechanism
      mem->free(*region, size);
      RETHROW(res);
    }
    obj->addRange(PhysPtr<void>::fromKernel(*region), size);
    return obj;
  }

} // namespace mythos
