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
#pragma once

#include "objects/IFrame.hh"
#include "objects/IKernelObject.hh"
#include "objects/FrameDataAmd64.hh"
#include "objects/IFactory.hh"
#include "objects/ops.hh"
#include "mythos/protocol/KernelObject.hh"

namespace mythos {

/** A frame that can be allocated from kernel memory and have arbitrary size.
 * The start address and size have to be 4KiB aligned.
 * Derived and referenced capabilities can further restrict the range
 * but can have to adhere to the power-of-two frame sizes.
 */
class MemoryRegion final
  : public IKernelObject, public IFrame
{
public:
  typedef protocol::Frame::FrameReq FrameReq;
  MemoryRegion(IAsyncFree* mem, PhysPtr<void> base, size_t size)
    : base(base), size(size), _mem(mem) {}

  MemoryRegion(const MemoryRegion&) = delete;

public: // IFrame interface
  Info getFrameInfo(Cap self) const override {
    FrameData c(self);
    return Info(c.getStart(base.physint()), self.isOriginal() ? size : c.getSize(), c.device, c.writable);
  }

public: // IKernelObject interface
  optional<void const*> vcast(TypeId id) const override {
    if (typeId<IFrame>() == id) return static_cast<const IFrame*>(this);
    THROW(Error::TYPE_MISMATCH);
  }

  optional<void> deleteCap(CapEntry&, Cap self, IDeleter& del) override {
    if (self.isOriginal()) { del.deleteObject(_deleteHandle); }
    RETURN(Error::SUCCESS);
  }

  void deleteObject(Tasklet* t, IResult<void>* r) override {
    _memdesc[0] = {base.log(), size};
    _memdesc[1] = {this, sizeof(MemoryRegion)};
    _mem->free(t, r, &_memdesc[0], &_memdesc[2]);
  }

  optional<Cap> mint(CapEntry&, Cap self, CapRequest request, bool derive) override {
    return FrameData::subRegion(self, base.physint(), size, FrameReq(request), derive);
  }

  Range<uintptr_t> addressRange(CapEntry&, Cap self) override { 
    FrameData c(self);
    return Range<uintptr_t>::bySize(c.getStart(base.physint()), self.isOriginal() ? size : c.getSize());
  }

  void invoke(Tasklet* t, Cap self, IInvocation* msg) override {
    Error err = Error::NOT_IMPLEMENTED;
    switch (msg->getProtocol()) {
    case protocol::Frame::proto:
      err = protocol::Frame::dispatchRequest(this, msg->getMethod(), t, self, msg);
      break;
    }
    if (err != Error::INHIBIT) msg->replyResponse(err);
  }

  Error frameInfo(Tasklet*, Cap self, IInvocation* msg){
    auto data = msg->getMessage()->write<protocol::Frame::Info>();
    FrameData c(self);
    data->addr = c.getStart(base.physint());
    data->size = self.isOriginal() ? size : c.getSize();
    data->device = c.device;
    data->writable = c.writable;
    return Error::SUCCESS;
  }

private:
  PhysPtr<void> base;
  size_t size;
  IAsyncFree* _mem;
  MemoryDescriptor _memdesc[2];
  IDeleter::handle_t _deleteHandle = {this};
};

class MemoryRegionFactory : public FactoryBase
{
public:
  static optional<MemoryRegion*>

  factory(CapEntry* dstEntry, CapEntry* memEntry, Cap memCap, IAllocator* mem,
    size_t size, size_t alignment);

  Error factory(CapEntry* dstEntry, CapEntry* memEntry, Cap memCap,
      IAllocator* mem, IInvocation* msg) const override {
    auto data = msg->getMessage()->cast<protocol::Frame::Create>();
    return factory(dstEntry, memEntry, memCap, mem, data->size, data->alignment).state();
  }
};


} // mythos
