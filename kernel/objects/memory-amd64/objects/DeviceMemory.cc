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
#include "objects/DeviceMemory.hh"

#include "util/assert.hh"
#include "util/align.hh"
#include "objects/IFrame.hh"
#include "objects/IKernelObject.hh"
#include "objects/CapEntry.hh"
#include "objects/ops.hh"
#include "objects/FrameDataAmd64.hh"
#include "mythos/protocol/Frame.hh"
#include "objects/mlog.hh"


mythos::DeviceMemory::DeviceMemory() 
{
  for (size_t i = 0; i < FrameSize::DEVICE_REGIONS; ++i) {
    _memory_region[i].setBase(FrameSize::REGION_MAX_SIZE * i);
  }
}

mythos::optional<void> mythos::DeviceMemory::deriveFrame(
    CapEntry& rootEntry, Cap rootCap, CapEntry& dstEntry,
    uintptr_t addr, size_t size, bool writable)
{
  // check position and alignment
  if (addr/FrameSize::REGION_MAX_SIZE >= FrameSize::DEVICE_REGIONS)
    THROW(Error::INSUFFICIENT_RESOURCES);
  if (!is_aligned(addr, align4K) || !is_aligned(size, align4K)) THROW(Error::UNALIGNED);
  auto sizeBits = FrameSize::frameSize2Bits(size);
  if (size != FrameSize::frameBits2Size(sizeBits)) THROW(Error::UNALIGNED);
  auto obj = &_memory_region[addr/FrameSize::REGION_MAX_SIZE];
  auto base = obj->base;
  ASSERT(base == round_down(addr, FrameSize::REGION_MAX_SIZE));

  MLOG_DETAIL(mlog::km, "create device frame in", DVAR(&dstEntry));
  if (!dstEntry.acquire()) RETURN(Error::LOST_RACE);

  // insert derived capability into the tree as child of the memory root object
  auto capData = FrameData().start(base, addr).sizeBits(sizeBits).device(true).writable(writable);
  auto res = cap::inherit(rootEntry, rootCap, dstEntry, Cap(obj, capData).asDerived());
  RETURN(res);
}

void mythos::DeviceMemory::invoke(Tasklet* t, Cap self, IInvocation* msg)
{
  Error err = Error::NOT_IMPLEMENTED;
  switch (msg->getProtocol()) {
  case protocol::DeviceMemory::proto:
    err = protocol::DeviceMemory::dispatchRequest(this, msg->getMethod(), t, self, msg);
    break;
  }
  if (err != Error::INHIBIT) msg->replyResponse(err);
}

mythos::Error mythos::DeviceMemory::invokeCreate(Tasklet*, Cap self, IInvocation* msg)
{
  auto ib = msg->getMessage();
  auto data = ib->read<protocol::DeviceMemory::Create>();
  MLOG_DETAIL(mlog::km, "create device frame", DVAR(data.dstSpace()), DVAR(data.dstPtr), 
              DVARhex(data.addr), DVARhex(data.size), DVAR(data.writable));
  
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
    // we drop the reference count here, this should be okay because we are in synchronous code. 
    // The CapMap containing the entry can not be deallocated until the end of this function.
  }

  auto res = deriveFrame(*msg->getCapEntry(), self, **dstEntry, data.addr, data.size, data.writable);
  return res.state();
}

void mythos::DeviceMemory::Region::invoke(Tasklet* t, Cap self, IInvocation* msg)
{
  Error err = Error::NOT_IMPLEMENTED;
  switch (msg->getProtocol()) {
  case protocol::Frame::proto:
	err = protocol::Frame::dispatchRequest(this, msg->getMethod(), t, self, msg);
	break;
  }
  if (err != Error::INHIBIT) msg->replyResponse(err);
}

mythos::Error mythos::DeviceMemory::Region::frameInfo(Tasklet*, Cap self, IInvocation* msg)
{
  auto data = msg->getMessage()->write<protocol::Frame::Info>();
  FrameData c(self);
  data->addr = c.getStart(base);
  data->size = self.isOriginal() ? FrameSize::REGION_MAX_SIZE : c.getSize();
  data->device = c.device;
  data->writable = c.writable;
  return Error::SUCCESS;
}
