/* -*- mode:C++; -*- */
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

#include "objects/IFrame.hh"
#include "objects/IKernelObject.hh"
#include "objects/Frame.hh"
#include "objects/FrameData.hh"
#include "objects/IFactory.hh"
#include "objects/ops.hh"
#include "mythos/protocol/KernelObject.hh"

namespace mythos {

/**
 * MemoryRegion to be allocated from the untyped memory.
 * The offset must be alligned by the maximum page size.
 */
class MemoryRegion final
  : public IKernelObject, public IFrame
{
public:
  typedef protocol::Frame::FrameReq FrameReq;
  MemoryRegion(IAsyncFree* mem, PhysPtr<void> start, size_t size);
  MemoryRegion(const MemoryRegion&) = delete;

public: // IFrame interface
  Info getFrameInfo(Cap self) const override {
    FrameData c(self);
    return {PhysPtr<void>(frame.start), size, c.writable};
  }

public: // IKernelObject interface
  optional<void const*> vcast(TypeId id) const override {
    if (TypeId::id<IFrame>() == id) return static_cast<const IFrame*>(this);
    return Error::TYPE_MISMATCH;
  }

  optional<void> deleteCap(Cap self, IDeleter& del) override {
    if (self.isOriginal()) { del.deleteObject(_deleteHandle); }
    return Error::SUCCESS;
  }

  void deleteObject(Tasklet* t, IResult<void>* r) override {
    _mem->free(t, r, &_memdesc[0], &_memdesc[2]);
  }

  optional<Cap> mint(Cap self, CapRequest request, bool derive) override {
    if (!derive) return FrameData(self).referenceRegion(self, FrameReq(request));
    else return FrameData(self).deriveRegion(self, frame, FrameReq(request), size);
  }

  Range<uintptr_t> addressRange(Cap) override { return {frame.start, frame.start+size}; }

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
    data->addr = frame.start;
    data->size = size;
    data->writable = c.writable;
    return Error::SUCCESS;
  }

private:
  Frame frame;
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
