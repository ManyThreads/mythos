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

  // check position and alignment
  if (data.addr/FrameSize::REGION_MAX_SIZE >= FrameSize::DEVICE_REGIONS)
    return Error::INSUFFICIENT_RESOURCES;
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
    dstEntry = dstEntryRef->entry;
    // we drop the reference count here, this should be okay because we are in synchronous code. 
    // The CapMap containing the entry can not be deallocated until the end of this function.
  }

  MLOG_DETAIL(mlog::km, "create device frame in", DVAR(*dstEntry));
  if (!dstEntry->acquire()) return Error::LOST_RACE;

  // insert derived capability into the tree as child of the memory root object
  auto capData = FrameData().start(base, data.addr).sizeBits(sizeBits).device(true).writable(data.writable);
  auto res = cap::inherit(*msg->getCapEntry(), self, **dstEntry, Cap(obj, capData).asDerived());

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
