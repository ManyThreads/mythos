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
 * Copyright 2019 Randolf Rotta, Robert Kuban, and contributors, BTU Cottbus-Senftenberg
 */
#pragma once

#include "util/assert.hh"
#include "objects/IFrame.hh"
#include "objects/IKernelObject.hh"
#include "objects/CapEntry.hh"
#include "objects/FrameDataAmd64.hh"
#include "mythos/protocol/Frame.hh"
#include "objects/mlog.hh"

namespace mythos {

  /** The global root of the resource tree */
  class DeviceMemory final
    : public IKernelObject
  {
  public:
    DeviceMemory();
    virtual ~DeviceMemory() {}

    void init() {
      ASSERT(isKernelAddress(this));
      rootEntry.initRoot(Cap(this)); 
    }

    /** used to attach the static memory regions as children. */
    CapEntry& get_cap_entry() { 
      ASSERT(isKernelAddress(this));
      return rootEntry; 
    }

    optional<void> deriveFrame(
        CapEntry& rootEntry, Cap rootCap, CapEntry& dstEntry,
        uintptr_t addr, size_t size, bool writable);

  public: // IKernelObject interface
    Range<uintptr_t> addressRange(CapEntry&, Cap) override  { return {0, ~uintptr_t(0)}; }
    optional<void const*> vcast(TypeId id) const override { 
        if (typeId<DeviceMemory>() == id) return this;
        THROW(Error::TYPE_MISMATCH);
    }
    optional<void> deleteCap(CapEntry&, Cap, IDeleter&) override { RETURN(Error::SUCCESS); }
    void invoke(Tasklet* t, Cap self, IInvocation* msg) override;
    Error invokeCreate(Tasklet*, Cap self, IInvocation* msg);

  public:
    /** helper memory region that describes a range of physical memory. */
    class Region final
      : public IFrame
    {
    public:
      typedef protocol::Frame::FrameReq FrameReq;
      Region() {}
      Region(const Region&) = delete;
      virtual ~Region() {}
      void setBase(uintptr_t base) { this->base = base; }

    public: // IFrame interface
      Info getFrameInfo(Cap self) const override {
        FrameData c(self);
        return Info(c.getStart(base), self.isOriginal() ? FrameSize::REGION_MAX_SIZE : c.getSize(), c.device, c.writable);
      }

    public: // IKernelObject interface
      optional<void const*> vcast(TypeId id) const override {
        if (typeId<IFrame>() == id) return static_cast<const IFrame*>(this);
        THROW(Error::TYPE_MISMATCH);
      }

      optional<void> deleteCap(CapEntry&, Cap, IDeleter&) override { RETURN(Error::SUCCESS); }

      optional<Cap> mint(CapEntry&, Cap self, CapRequest request, bool derive) override {
        return FrameData::subRegion(self, base, FrameSize::REGION_MAX_SIZE, FrameReq(request), derive);
      }

      Range<uintptr_t> addressRange(CapEntry&, Cap self) override { 
        FrameData c(self);
        return Range<uintptr_t>::bySize(c.getStart(base), self.isOriginal() ? FrameSize::REGION_MAX_SIZE : c.getSize());
      }

      void invoke(Tasklet* t, Cap self, IInvocation* msg) override;
      Error frameInfo(Tasklet*, Cap self, IInvocation* msg);
    public:
      uintptr_t base;
    };

  protected:
    CapEntry rootEntry;
    Region _memory_region[FrameSize::DEVICE_REGIONS];
  };

} // namespace mythos
