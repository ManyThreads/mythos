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

#include "util/assert.hh"
#include "objects/IFrame.hh"
#include "objects/IKernelObject.hh"
#include "objects/CapEntry.hh"
#include "objects/ops.hh"
#include "objects/FrameDataAmd64.hh"
#include "mythos/protocol/Frame.hh"
#include "objects/mlog.hh"

namespace mythos {

  /** The global root of the resource tree */
  class MemoryRoot final
    : public IKernelObject
  {
  public:
    MemoryRoot() {
      for (size_t i = 0; i < FrameSize::STATIC_MEMORY_REGIONS; ++i) {
        _memory_region[i].setBase(FrameSize::REGION_MAX_SIZE * i);
      }
    }

    void init() {
      ASSERT(isKernelAddress(this));
      rootEntry.initRoot(Cap(this)); 
    }

    /** used to attach the static memory regions as children. */
    CapEntry& getRoot() { 
      ASSERT(isKernelAddress(this));
      return rootEntry; 
    }

  public: // IKernelObject interface
    Range<uintptr_t> addressRange(CapEntry&, Cap) override  { return {0, ~uintptr_t(0)}; }
    optional<void const*> vcast(TypeId) const override { THROW(Error::TYPE_MISMATCH); }
    optional<void> deleteCap(CapEntry&, Cap, IDeleter&) override { RETURN(Error::SUCCESS); }

    void invoke(Tasklet* t, Cap self, IInvocation* msg) override {
        Error err = Error::NOT_IMPLEMENTED;
        switch (msg->getProtocol()) {
        case protocol::MemoryRoot::proto:
          err = protocol::MemoryRoot::dispatchRequest(this, msg->getMethod(), t, self, msg);
          break;
        }
        if (err != Error::INHIBIT) msg->replyResponse(err);
    }

    Error invokeCreate(Tasklet*, Cap self, IInvocation* msg){
      auto ib = msg->getMessage();
      auto data = ib->read<protocol::MemoryRoot::Create>();
      MLOG_DETAIL(mlog::km, "create device frame", DVAR(data.dstSpace()), DVAR(data.dstPtr), 
                  DVARhex(data.addr), DVARhex(data.size), DVAR(data.writable));

      // check position and alignment
      if (data.addr/FrameSize::REGION_MAX_SIZE >= FrameSize::STATIC_MEMORY_REGIONS) return Error::INSUFFICIENT_RESOURCES;
      if (data.addr % 4096 != 0 || data.size % 4096 != 0) return Error::UNALIGNED;
      // TODO disallow access to the 4GiB kernel memory area
      auto sizeBits = FrameSize::frameSize2Bits(data.size);
      if (data.size != FrameSize::frameBits2Size(sizeBits)) return Error::UNALIGNED;
      auto obj = &_memory_region[data.addr/FrameSize::REGION_MAX_SIZE];
      auto base = obj->base;
      ASSERT(base == (data.addr/FrameSize::REGION_MAX_SIZE)*FrameSize::REGION_MAX_SIZE);

      optional<CapEntry*> dstEntry;
      if (data.dstSpace() == null_cap) { // direct address
        dstEntry = msg->lookupEntry(data.dstPtr, 32, true); // lookup for write access
        if (!dstEntry) return dstEntry.state();
      } else { // indirect address
        TypedCap<ICapMap> dstSpace(msg->lookupEntry(data.dstSpace()));
        if (!dstSpace) return dstSpace.state();
        auto dstEntryRef = dstSpace.lookup(data.dstPtr, data.dstDepth, true); // lookup for write
        if (!dstEntryRef) return dstEntryRef.state();
        dstEntry = dstEntryRef->entry; // TODO should be move, is reference counting correctly done?
      }

      MLOG_DETAIL(mlog::km, "create device frame in", DVAR(*dstEntry));
      if (!dstEntry->acquire()) return Error::LOST_RACE;

      // insert derived capability into the tree as child of the memory root object
      auto capData = FrameData().start(base, data.addr).sizeBits(sizeBits).kernel(false).writable(data.writable);
      auto res = cap::inherit(*msg->getCapEntry(), self, **dstEntry, Cap(obj, capData).asDerived());

      return res.state();
    }

  public:
    /** helper memory region that describes a range of physical memory. */
    class StaticMemoryRegion final
      : public IKernelObject, public IFrame
    {
    public:
      typedef protocol::Frame::FrameReq FrameReq;
      StaticMemoryRegion() {}
      StaticMemoryRegion(const StaticMemoryRegion&) = delete;
      void setBase(uintptr_t base) { this->base = base; }

    public: // IFrame interface
      Info getFrameInfo(Cap self) const override {
        FrameData c(self);
        return Info(c.getStart(base), self.isOriginal() ? size : c.getSize(), c.kernel, c.writable);
      }

    public: // IKernelObject interface
      optional<void const*> vcast(TypeId id) const override {
        if (typeId<IFrame>() == id) return static_cast<const IFrame*>(this);
        THROW(Error::TYPE_MISMATCH);
      }

      optional<void> deleteCap(CapEntry&, Cap, IDeleter&) override { RETURN(Error::SUCCESS); }

      optional<Cap> mint(CapEntry&, Cap self, CapRequest request, bool derive) override {
        return FrameData::subRegion(self, base, size, FrameReq(request), derive);
      }

      Range<uintptr_t> addressRange(CapEntry&, Cap self) override { 
        FrameData c(self);
        return Range<uintptr_t>::bySize(c.getStart(base), self.isOriginal() ? size : c.getSize());
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
        data->addr = c.getStart(base);
        data->size = self.isOriginal() ? size : c.getSize();
        data->kernel = c.kernel;
        data->writable = c.writable;
        return Error::SUCCESS;
      }

    public:
      uintptr_t base;
      constexpr static size_t size = {FrameSize::REGION_MAX_SIZE};
    };

  protected:
    CapEntry rootEntry;
    StaticMemoryRegion _memory_region[FrameSize::STATIC_MEMORY_REGIONS];
  };

} // namespace mythos
