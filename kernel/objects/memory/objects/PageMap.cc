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
#include "objects/PageMap.hh"

#include "util/assert.hh"
#include "util/mstring.hh"
#include "objects/ops.hh"
#include "objects/TypedCap.hh"
#include "objects/FrameData.hh"
#include "objects/PML4InvalidationBroadcast.hh"
#include "boot/pagetables.hh"
#include "objects/mlog.hh"

namespace mythos {

  PageMap::PageMap(IAsyncFree* mem, size_t level, void* tableMem, void* capMem)
    : _level(level), _mem(mem), _deletionSink(nullptr)
  {
    ASSERT(Alignment<FrameSize::MIN_FRAME_SIZE>::is_aligned(tableMem));
    _memDesc[0] = MemoryDescriptor(tableMem, FrameSize::MIN_FRAME_SIZE);
    _memDesc[1] = MemoryDescriptor(capMem, sizeof(CapEntry)*num_caps());
    _memDesc[2] = MemoryDescriptor(this, sizeof(PageMap));
    memset(_memDesc[0].ptr, 0, _memDesc[0].size);
    memset(_memDesc[1].ptr, 0, _memDesc[1].size);
    if (isRootMap()) {
      memcpy(&_pm_table(TABLE_SIZE/2), &boot::pml4_table[TABLE_SIZE/2],
             TABLE_SIZE/2*sizeof(PageTableEntry));
    }
    uint32_t pmPtr = kernel2phys(static_cast<IPageMap*>(this)); // 29 bits, 8 byte aligned
    _pm_table(0).pmPtr = (pmPtr>>3) & 0x3FF; // lowest 10bit
    _pm_table(1).pmPtr = (pmPtr>>13) & 0x3FF; // middle 10bit
    _pm_table(2).pmPtr = (pmPtr>>23) & 0x3FF; // highest 10bit
  }

  IPageMap* PageMap::table2PageMap(PageTableEntry const* table) {
    uintptr_t ptr = (table[0].pmPtr<<3) | (table[1].pmPtr<<13) | (table[2].pmPtr<<23);
    return phys2kernel<IPageMap>(ptr);
  }

  Range<uintptr_t> PageMap::addressRange(Cap self)
  {
    PageMapData data(self);
    if (!data.mapped) {
      return Range<uintptr_t>::bySize(
          PhysPtr<void>::fromKernel(_memDesc[0].ptr).physint(),
          _memDesc[0].size);
    } else {
      // this works because the range of frames is the actual physical frame
      // and the range of the page table includes the actual table frame.
      auto addr = _pm_table(data.index).getAddr();
      return {addr, addr+1};
    }
  }

  void PageMap::print() const
  {
    MLOG_ERROR(mlog::cap, "page map level", level(), &_pm_table(0));
    for (size_t i = 0; i < num_caps(); ++i) MLOG_INFO(mlog::cap, i, _pm_table(i), _cap_table(i));
    for (size_t i = num_caps(); i < TABLE_SIZE; ++i) MLOG_INFO(mlog::cap, i, _pm_table(i));
  }

  optional<void> PageMap::deleteCap(Cap self, IDeleter& del)
  {
    if (PageMapData(self).mapped) {
      MLOG_DETAIL(mlog::cap, "delete mapped Frame or PageMap", PageMapData(self).index);
      _pm_table(PageMapData(self).index).reset();
    } else {
      if (self.isOriginal()) {
        MLOG_DETAIL(mlog::cap, "delete PageMap", self);
        for (size_t i = 0; i < num_caps(); ++i) {
          auto res = del.deleteEntry(_cap_table(i));
          ASSERT_MSG(res, "Mapped entries must be deletable.");
          del.deleteObject(del_handle);
        }
      }
    }
    RETURN(Error::SUCCESS);
  }

  void PageMap::response(Tasklet* t, optional<void> res)
  {
    if (isRootMap()) {
      PANIC_MSG(res, "Invalidation failed.");
      _mem->free(t, _deletionSink, _memDesc, &_memDesc[3]);
    } else OOPS_MSG(false, "Non-PML4 pagemap recieved response.");
  }

  void PageMap::deleteObject(Tasklet* t, IResult<void>* r)
  {
    ASSERT(r);
    monitor.doDelete(t, [=](Tasklet* t){
        if (isRootMap()) {
          _deletionSink = r;
          PML4InvalidationBroadcast::run(t, this, PhysPtr<void>::fromKernel(&_pm_table(0)));
        } else _mem->free(t, r, _memDesc, &_memDesc[3]);
      });
  }

  optional<void> PageMap::mapTable(CapEntry* tableEntry, MapFlags req, size_t index)
  {
    MLOG_INFO(mlog::cap, "mapTable", DVAR(level()), DVAR(tableEntry), DVAR(index));
    ASSERT(index < num_caps());
    TypedCap<IPageMap> table(tableEntry);
    if (!table) RETHROW(table);
    auto info = table->getPageMapInfo(table.cap());
    if (level() != info.level+1) THROW(Error::INVALID_CAPABILITY);
    MLOG_DETAIL(mlog::cap, "mapTable checks done", DVAR(level()), DVAR(table.cap()), DVAR(index));

    auto pme = &_pm_table(index);
    auto entry = PageTableEntry().present(true).userMode(true).writeable(req.writable)
      .writeThrough(req.write_through).cacheDisabled(req.cache_disabled)
      .withAddr(info.table.physint())
      .pmPtr(pme->pmPtr)
      .configurable(info.configurable & req.configurable);

    // inherit
    auto newCap = PageMapData::mapTable(this, table.cap(), entry.configurable, index);
    cap::resetReference([=]{ pme->reset(); }, _cap_table(index));
    return cap::setReference([=,&entry]{ pme->set(entry); },
                            _cap_table(index), newCap, *tableEntry, table.cap());
  }

  optional<Cap> PageMap::mint(Cap self, CapRequest request, bool)
  {
    return PageMapData(self).mint(self, PageMapReq(request));
  }

  PageMap::MmapOp::MmapOp(uintptr_t vaddr, size_t vsize,
                             optional<CapEntry*> frameEntry, MapFlags flags)
    : FrameOp(vaddr, vsize)
  {
    frameOp.flags = flags;
    frameOp.frame = TypedCap<IFrame>(frameEntry);
    if (frameEntry) frameOp.frameEntry = *frameEntry;
    if (frameOp.frame) frameOp.frameInfo = frameOp.frame.getFrameInfo();
  }

  optional<void> PageMap::MmapOp::applyFrame(PageTableEntry* table, size_t index)
  {
    return table2PageMap(table)->mapFrame(index, frameOp, offset());
  }

  optional<void> PageMap::mapFrame(size_t index, IPageMap::FrameOp const& op, size_t offset)
  {
    MLOG_INFO(mlog::cap, "mapFrame", DVAR(level()), DVAR(op.frame.cap()), DVAR(index), DVAR(offset),
                  DVARhex(op.frameInfo.start.physint()), DVARhex(op.frameInfo.size));
    ASSERT(op.frame);
    ASSERT(index < num_caps());
    // check sizes and alignment
    uintptr_t frameaddr = op.frameInfo.start.physint() + offset;
    if (frameaddr % pageSize() != 0) THROW(Error::PAGEMAP_MISSING);
    if (offset + pageSize() > op.frameInfo.size) THROW(Error::INSUFFICIENT_RESOURCES);
    MLOG_DETAIL(mlog::cap, "mapFrame checks done");

    // build entry: page flag only valid on PML2 and higher
    /// @todo what about W^X ?
    /// @todo what about write_combine? (PAT/PAT2)
    auto pme = &_pm_table(index);
    auto entry = PageTableEntry().present(true).userMode(true).page(level() > 1)
      .writeable(op.flags.writable && op.frameInfo.writable)
      //.executeDisabled(!req.executable) // not working on KNC
      .writeThrough(op.flags.write_through)
      .cacheDisabled(op.flags.cache_disabled)
      .withAddr(frameaddr)
      .configurable(op.frameInfo.writable)
      .pmPtr(pme->pmPtr);
    auto newCap = PageMapData::mapFrame(this, op.frame.cap(), op.frameInfo, index);

    // inherit
    cap::resetReference([=]{ pme->reset(); }, _cap_table(index));
    return cap::setReference([=,&entry]{ pme->set(entry); },
                            _cap_table(index), newCap, *op.frameEntry, op.frame.cap());
  }

  optional<void> PageMap::MunmapOp::applyFrame(PageTableEntry* table, size_t index)
  {
    return table2PageMap(table)->unmapEntry(index);
  }

  optional<void> PageMap::unmapEntry(size_t index)
  {
    MLOG_INFO(mlog::cap, "unmapEntry", DVAR(level()), DVAR(index));
    ASSERT(index < num_caps());
    auto pme = &_pm_table(index);
    cap::resetReference([=]{ pme->reset(); }, _cap_table(index)); // ignore lost races
    RETURN(Error::SUCCESS);
  }

  optional<void> PageMap::MprotectOp::applyFrame(PageTableEntry* table, size_t index)
  {
    MLOG_INFO(mlog::cap, "mprotect", DVAR(index), DVARhex(flags.value));
    PageTableEntry pme = table[index]; // copy the entry for later compare_exchange!
    ASSERT(pme.page); // otherwise we should not have ended up here
    auto entry = pme.writeable(flags.writable && pme.configurable)
      //.executeDisabled(!req.executable) // not working on KNC
      .writeThrough(flags.write_through)
      .cacheDisabled(flags.cache_disabled);
    table[index].replace(pme, entry); // simply ignore lost races
    RETURN(Error::SUCCESS);
  }

  template<>
  optional<void> PageMap::operateFrame<1>(PageTableEntry* table, FrameOp& op)
  {
    size_t startIndex = (op.vaddr()/pageSize(1)) % 512; // the part inside current table
    size_t numEntries = (op.vaddr()+op.vsize()+pageSize(1)-1)/pageSize(1) - op.vaddr()/pageSize(1);
    if (numEntries > 512-startIndex) numEntries = 512-startIndex;
    for (size_t i=0; i < numEntries; i++) {
      op.level = 1;
      ASSERT(Align4k::is_aligned(op.vaddr())); // checked in invoke*()
      ASSERT(op.vsize() >= Align4k::alignment()); // checked in invoke*()
      auto res = op.applyFrame(table, startIndex+i);
      if (!res) return res;
      op.moveForward(pageSize(1));
    }
    RETURN(Error::SUCCESS);
  }

  template<size_t LEVEL>
  optional<void> PageMap::operateFrame(PageTableEntry* table, FrameOp& op)
  {
    size_t startIndex = (op.vaddr()/pageSize(LEVEL)) % 512; // the part inside current table
    size_t numEntries = (op.vaddr()+op.vsize()+pageSize(LEVEL)-1)/pageSize(LEVEL) - op.vaddr()/pageSize(LEVEL);
    if (numEntries > 512-startIndex) numEntries = 512-startIndex;
    for (size_t i=0; i < numEntries; i++) {
      op.level = LEVEL;
      auto pme = table[startIndex+i];
      MLOG_DETAIL(mlog::cap, "operateFrame", DVARhex(op.vaddr()), DVARhex(op.vsize()),
                  DVARhex(op.offset()), DVAR(op.level));
      if (pme.present && !pme.page) { // recurse into lower page map
        if (!pme.configurable) THROW(Error::PAGEMAP_NOCONF); /// @todo or just skip?
        auto res = operateFrame<LEVEL-1>(phys2kernel<PageTableEntry>(pme.getAddr()), op);
        if (!res) RETHROW(res);
        continue; // don't do moveForward(), was done by LEVEL-1
      } else if (pme.present && pme.page) { // apply frame operator on mapped page
        if (LEVEL == 2) { // check if the operation is just on a part of the 2MiB page
          if (!Align2M::is_aligned(op.vaddr())) THROW(Error::PAGEMAP_MISSING);
          if (op.vsize() < Align2M::alignment()) THROW(Error::PAGEMAP_MISSING);
        }
        auto res = op.applyFrame(table, startIndex+i);
        if (!res) RETHROW(res);
      } else if (!op.skip_nonmapped) { // nothing mapped here but should be
        if (LEVEL > 2) THROW(Error::PAGEMAP_MISSING); // no pages possible in PML3 & PML4
        if (LEVEL == 2) { // check if the operation is just on a part of the 2MiB page
          if (!Align2M::is_aligned(op.vaddr())) THROW(Error::PAGEMAP_MISSING);
          if (op.vsize() < Align2M::alignment()) THROW(Error::PAGEMAP_MISSING);
        }
        auto res = op.applyFrame(table, startIndex+i);
        if (!res) RETHROW(res);
      }
      // move forward one page, but correct vaddr starting inside the page
      op.moveForward(pageSize(LEVEL) - op.vaddr() % pageSize(LEVEL));
    }
    RETURN(Error::SUCCESS);
  }

  optional<void> PageMap::operateFrame(FrameOp& op, InvocationBuf* msg)
  {
    if (op.vaddr() / pageSize() != 0
        || !Align4k::is_aligned(op.vaddr())
        || !Align4k::is_aligned(op.vsize())) {
      msg->write<protocol::PageMap::Result>(op.vaddr(), op.vsize(), level());
      THROW(Error::UNALIGNED);
    }

    optional<void> res;
    switch (level()) {
    case 1: res = operateFrame<1>(&_pm_table(0), op);
    case 2: res = operateFrame<2>(&_pm_table(0), op);
    case 3: res = operateFrame<3>(&_pm_table(0), op);
    case 4: res = operateFrame<4>(&_pm_table(0), op);
    default: break;
    }
    msg->write<protocol::PageMap::Result>(op.vaddr(), op.vsize(), op.level);
    return res;
  }

  optional<void> PageMap::InstallMapOp::applyTable(IPageMap* map, size_t index)
  {
    return map->mapTable(mapEntry, flags, index);
  }

  optional<void> PageMap::RemoveMapOp::applyTable(IPageMap* map, size_t index)
  {
    return map->unmapEntry(index);
  }

  template<>
  optional<void> PageMap::operateTable<1>(PageTableEntry* table, TableOp& op)
  {
    size_t index = op.vaddr/pageSize(1) % 512; // the part inside current table
    return op.applyTable(table2PageMap(table), index);
  }

  template<size_t LEVEL>
  optional<void> PageMap::operateTable(PageTableEntry* table, TableOp& op)
  {
    size_t index = op.vaddr/pageSize(LEVEL) % 512; // the part inside current table
    op.level = LEVEL;
    auto pme = table[index];
    if (LEVEL == 4 && index >= 128) THROW(Error::REQUEST_DENIED);
    if (op.tgtLevel == LEVEL) { // apply
      RETURN(op.applyTable(table2PageMap(table), index));
    } else if (pme.present && !pme.page) { // recurse into lower page map
      if (!pme.configurable) THROW(Error::PAGEMAP_NOCONF);
      RETURN(operateTable<LEVEL-1>(phys2kernel<PageTableEntry>(pme.getAddr()), op));
    } else THROW(Error::PAGEMAP_MISSING);
  }

  optional<void> PageMap::operateTable(TableOp& op, InvocationBuf* msg)
  {
    if (op.tgtLevel<1 || op.tgtLevel>4
        || op.vaddr / pageSize() != 0 || op.vaddr % pageSize(op.tgtLevel) != 0) {
      msg->write<protocol::PageMap::Result>(op.vaddr, 0u, level());
      THROW(Error::UNALIGNED);
    }

    optional<void> res;
    switch (level()) {
    case 1: res = operateTable<1>(&_pm_table(0), op);
    case 2: res = operateTable<2>(&_pm_table(0), op);
    case 3: res = operateTable<3>(&_pm_table(0), op);
    case 4: res = operateTable<4>(&_pm_table(0), op);
    default: break;
    }
    msg->write<protocol::PageMap::Result>(op.vaddr, 0u, op.level);
    return res;
  }

  void PageMap::invoke(Tasklet* t, Cap self, IInvocation* msg)
  {
    monitor.request(t, [=](Tasklet* t){
        Error err = Error::NOT_IMPLEMENTED;
        switch (msg->getProtocol()) {
        case protocol::PageMap::proto:
          err = protocol::PageMap::dispatchRequest(this, msg->getMethod(), t, self, msg);
          break;
        }
        if (err != Error::INHIBIT) {
          msg->replyResponse(err);
          monitor.requestDone();
        }
      } );
  }

  Error PageMap::invokeMmap(Tasklet*, Cap self, IInvocation* msg)
  {
    PageMapData pd(self);
    if (!pd.writable) return Error::REQUEST_DENIED;
    auto data = msg->getMessage()->read<protocol::PageMap::Mmap>();
    MmapOp op(data.vaddr, data.size, msg->lookupEntry(data.tgtFrame()), data.flags);
    if (!op.frameOp.frame) return op.frameOp.frame.state();
    if (op.vsize() > op.frameOp.frameInfo.size) return Error::INSUFFICIENT_RESOURCES;
    // note: unaligned frames cannot exist by construction
    ASSERT(Align4k::is_aligned(op.frameOp.frameInfo.start.physint()));
    return operateFrame(op, msg->getMessage()).state();
  }

  Error PageMap::invokeRemap(Tasklet*, Cap self, IInvocation*)
  {
    PageMapData pd(self);
    if (!pd.writable) return Error::REQUEST_DENIED;
    return Error::NOT_IMPLEMENTED;
  }

  Error PageMap::invokeMunmap(Tasklet*, Cap self, IInvocation* msg)
  {
    PageMapData pd(self);
    if (!pd.writable) return Error::REQUEST_DENIED;
    auto data = msg->getMessage()->read<protocol::PageMap::Munmap>();
    MunmapOp op(data.vaddr, data.size);
    return operateFrame(op, msg->getMessage()).state();
  }

  Error PageMap::invokeMprotect(Tasklet*, Cap self, IInvocation* msg)
  {
    PageMapData pd(self);
    if (!pd.writable) return Error::REQUEST_DENIED;
    auto data = msg->getMessage()->read<protocol::PageMap::Mprotect>();
    MprotectOp op(data.vaddr, data.size, data.flags);
    return operateFrame(op, msg->getMessage()).state();
  }

  Error PageMap::invokeInstallMap(Tasklet*, Cap self, IInvocation* msg)
  {
    PageMapData pd(self);
    if (!pd.writable) return Error::REQUEST_DENIED;
    auto data = msg->getMessage()->read<protocol::PageMap::InstallMap>();
    auto mapEntry = msg->lookupEntry(data.pagemap());
    if (!mapEntry) return Error::INVALID_CAPABILITY;
    InstallMapOp op(data.vaddr, data.level, *mapEntry, data.flags);
    return operateTable(op, msg->getMessage()).state();
  }

  Error PageMap::invokeRemoveMap(Tasklet*, Cap self, IInvocation* msg)
  {
    PageMapData pd(self);
    if (!pd.writable) return Error::REQUEST_DENIED;
    auto data = msg->getMessage()->read<protocol::PageMap::RemoveMap>();
    RemoveMapOp op(data.vaddr, data.level);
    return operateTable(op, msg->getMessage()).state();
  }

  optional<PageMap*>
  PageMapFactory::factory(CapEntry* dstEntry, CapEntry* memEntry, Cap memCap, IAllocator* mem,
                         size_t level)
  {
    if (!(level>=1 && level<=4)) THROW(Error::INVALID_ARGUMENT);
    auto table = mem->alloc(FrameSize::MIN_FRAME_SIZE, FrameSize::MIN_FRAME_SIZE);
    if (!table) RETHROW(table);
    auto caps = mem->alloc(sizeof(CapEntry)*PageMap::num_caps(level), FrameSize::MIN_FRAME_SIZE);
    if (!caps) {
      mem->free(*table, FrameSize::MIN_FRAME_SIZE);
      RETHROW(caps);
    }
    auto obj = mem->create<PageMap>(level, *table, *caps);
    if (!obj) {
      mem->free(*table, FrameSize::MIN_FRAME_SIZE);
      mem->free(*caps, sizeof(CapEntry)*PageMap::num_caps(level));
      RETHROW(obj);
    }
    auto cap = Cap(*obj).withData(PageMapData());
    auto res = cap::inherit(*memEntry, *dstEntry, memCap, cap);
    if (!res) {
      mem->free(*obj); // mem->release(obj) goes throug IKernelObject deletion mechanism
      mem->free(*table, FrameSize::MIN_FRAME_SIZE);
      mem->free(*caps, sizeof(CapEntry)*PageMap::num_caps(level));
      RETHROW(res);
    }
    return *obj;
  }

} // namespace mythos
