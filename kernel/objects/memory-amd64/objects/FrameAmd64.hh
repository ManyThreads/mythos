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
#include "mythos/protocol/Frame.hh"

namespace mythos {

class Frame final
  : public IKernelObject
  , public IFrame
{
public:
  typedef protocol::Frame::FrameReq FrameReq;
  Frame() : start(0) {}
  Frame(const Frame&) = delete;

public: // IFrame interface
  Info getFrameInfo(Cap self) const override {
    FrameData c(self);
    return {PhysPtr<void>(c.addr(start)), c.size(), c.writable};
  }

public: // IKernelObject interface
  optional<void const*> vcast(TypeId id) const override {
    if (id == typeId<IFrame>()) return static_cast<IFrame const*>(this);
    THROW(Error::TYPE_MISMATCH);
  }

  optional<Cap> mint(CapEntry&, Cap self, CapRequest request, bool derive) override {
    if (!derive) return FrameData(self).referenceFrame(self, FrameReq(request));
    else THROW(Error::INVALID_REQUEST);
  }

  Range<uintptr_t> addressRange(CapEntry&, Cap self) override {
    return {FrameData(self).addr(start), FrameData(self).end(start)};
  }

  optional<void> deleteCap(CapEntry&, Cap, IDeleter&) override { RETURN(Error::SUCCESS); };

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
    data->addr = c.addr(start);
    data->size = c.size();
    data->writable = c.writable;
    return Error::SUCCESS;
  }

public:
  uintptr_t start;
};

} // mythos

