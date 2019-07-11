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
#include "objects/ops.hh"
#include "mythos/protocol/KernelObject.hh"

namespace mythos {

/**
 * memory region that describes a 1TB region of physical memory.
 * to be allovcatated statically and to be given to the in init process.
 */
class StaticMemoryRegion final
  : public IKernelObject, public IFrame
{
public:
  typedef protocol::Frame::FrameReq FrameReq;
  constexpr static size_t size = {FrameSize::REGION_MAX_SIZE};
  StaticMemoryRegion() : _ownRoot(Cap::asAllocated()) {}
  StaticMemoryRegion(const StaticMemoryRegion&) = delete;

  /** used to attach this region to the resource root */
  optional<void> init(uintptr_t base, CapEntry& root) {
    this->base = PhysPtr<void>(base);
    return cap::inherit(root, root.cap(), _ownRoot, Cap(this,FrameData().writable(true).kernel(false)));
  }

  /** used to attach the root KernelMemory as child of a static memory region. */
  CapEntry& getRoot() { return _ownRoot; }

public: // IFrame interface
  Info getFrameInfo(Cap self) const override {
    FrameData c(self);
    return Info(c.getStart(base.physint()), self.isOriginal() ? size : c.getSize(), c.kernel, c.writable);
  }

public: // IKernelObject interface
  optional<void const*> vcast(TypeId id) const override {
    if (typeId<IFrame>() == id) return static_cast<const IFrame*>(this);
    THROW(Error::TYPE_MISMATCH);
  }

  optional<void> deleteCap(CapEntry&, Cap, IDeleter&) override { RETURN(Error::SUCCESS); }

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
    data->kernel = c.kernel;
    data->writable = c.writable;
    return Error::SUCCESS;
  }

private:
  PhysPtr<void> base;
  CapEntry _ownRoot;
};

} // namespace mythos
