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
#include "objects/CapEntry.hh"
#include "objects/ops.hh"

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
  static constexpr size_t SIZE = FrameSize::MAX_REGION_SIZE;
  StaticMemoryRegion() : _ownRoot(Cap::asAllocated()) {}
  StaticMemoryRegion(const StaticMemoryRegion&) = delete;

  /** used to attach this region to the resource root */
  optional<void> init(size_t index, CapEntry& root) {
    frame.start = index*SIZE;
    return cap::inherit(root, _ownRoot, root.cap(), Cap(this,FrameData()));
  }

  /** used to attach the root UntypedMemory as child of a static memory region. */
  CapEntry& getRoot() { return _ownRoot; }

public: // IFrame interface
  Info getFrameInfo(Cap self) const override {
    FrameData c(self);
    return {PhysPtr<void>(frame.start), SIZE, c.writable};
  }

public: // IKernelObject interface
  optional<void const*> vcast(TypeId id) const override {
    if (TypeId::id<IFrame>() == id) return static_cast<const IFrame*>(this);
    THROW(Error::TYPE_MISMATCH);
  }

  optional<void> deleteCap(Cap, IDeleter&) override { RETURN(Error::SUCCESS); }

  optional<Cap> mint(Cap self, CapRequest request, bool derive) override {
    if (!derive) return FrameData(self).referenceRegion(self, FrameReq(request));
    else return FrameData(self).deriveRegion(self, frame, FrameReq(request), SIZE);
  }

  Range<uintptr_t> addressRange(Cap) override { return {frame.start, frame.start+SIZE}; }

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
    data->size = SIZE;
    data->writable = c.writable;
    return Error::SUCCESS;
  }

private:
  Frame frame;
  CapEntry _ownRoot;
};

} // namespace mythos
