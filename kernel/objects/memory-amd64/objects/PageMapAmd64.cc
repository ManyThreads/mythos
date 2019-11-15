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
#include "objects/PageMapAmd64.hh"

#include "util/assert.hh"
#include "util/mstring.hh"
#include "objects/ops.hh"
#include "objects/TypedCap.hh"
#include "objects/FrameDataAmd64.hh"
#include "objects/PML4InvalidationBroadcastAmd64.hh"
#include "boot/pagetables.hh"
#include "objects/mlog.hh"

namespace mythos {

  PageMap::PageMap(IAsyncFree* mem, size_t level, void* tableMem, void* capMem)
    : _level(level), _mem(mem), _deletionSink(nullptr)
  {
    ASSERT(Alignment<FrameSize::PAGE_MIN_SIZE>::is_aligned(tableMem));
    _memDesc[0] = MemoryDescriptor(tableMem, FrameSize::PAGE_MIN_SIZE);
    _memDesc[1] = MemoryDescriptor(capMem, sizeof(CapEntry)*num_caps());
    _memDesc[2] = MemoryDescriptor(this, sizeof(PageMap));
    memset(_memDesc[0].ptr, 0, _memDesc[0].size);
    memset(_memDesc[1].ptr, 0, _memDesc[1].size);
    if (isRootMap()) {
      memcpy(&_pm_table(TABLE_SIZE/2), &boot::pml4_table[TABLE_SIZE/2],
             TABLE_SIZE/2*sizeof(PageTableEntry));
    }
    uint32_t pmPtr = kernel2offset(static_cast<IPageMap*>(this)); // 29 bits, 8 byte aligned
    _pm_table(0).pmPtr = (pmPtr>>3) & 0x3FF; // lowest 10bit
    _pm_table(1).pmPtr = (pmPtr>>13) & 0x3FF; // middle 10bit
    _pm_table(2).pmPtr = (pmPtr>>23) & 0x3FF; // highest 10bit
  }

  IPageMap* PageMap::table2PageMap(PageTableEntry const* table) {
    auto ptr = uint32_t((table[0].pmPtr<<3) | (table[1].pmPtr<<13) | (table[2].pmPtr<<23));
    return offset2kernel<IPageMap>(ptr);
  }

  Range<uintptr_t> PageMap::MappedFrame::addressRange(CapEntry& entry, Cap)
  {
    auto idx = &entry - &map->_cap_table(0);  // index into the capability table
    auto ptr = map->_pm_table(idx).getAddr();
    MLOG_DETAIL(mlog::cap, "MappedFrame::addressRange", DVAR(&entry), DVAR(idx), DVARhex(ptr), DVARhex(map->pageSize()));
    return Range<uintptr_t>::bySize(ptr, map->pageSize());
  }

  Range<uintptr_t> PageMap::MappedPageMap::addressRange(CapEntry& entry, Cap)
  {
    auto idx = &entry - &map->_cap_table(0);
    auto addr = map->_pm_table(idx).getAddr();
    return Range<uintptr_t>::bySize(addr, 4096); // the x86 page table is 4K large
  }

  Range<uintptr_t> PageMap::addressRange(CapEntry&, Cap)
  {
    return Range<uintptr_t>::bySize(
        PhysPtr<void>::fromKernel(_memDesc[0].ptr).physint(),
        4096); // 4K large, see MappedPageMap::addressRange() above
  }

  void PageMap::print() const
  {
    MLOG_ERROR(mlog::cap, "page map level", level(), &_pm_table(0));
    for (size_t i = 0; i < num_caps(); ++i) MLOG_INFO(mlog::cap, i, _pm_table(i), _cap_table(i));
    for (size_t i = num_caps(); i < TABLE_SIZE; ++i) MLOG_INFO(mlog::cap, i, _pm_table(i));
  }

  optional<void> PageMap::MappedFrame::deleteCap(CapEntry& entry, Cap /*self*/, IDeleter&)
  {
    auto idx = &entry - &map->_cap_table(0);
    MLOG_DETAIL(mlog::cap, "delete mapped Frame", DVAR(idx));
    map->_pm_table(idx).reset();
    RETURN(Error::SUCCESS);
  }

  optional<void> PageMap::MappedPageMap::deleteCap(CapEntry& entry, Cap /*self*/, IDeleter&)
  {
    auto idx = &entry - &map->_cap_table(0);
    MLOG_DETAIL(mlog::cap, "delete mapped PageMap", DVAR(idx));
    map->_pm_table(idx).reset();
    RETURN(Error::SUCCESS);
  }

  optional<void> PageMap::deleteCap(CapEntry&, Cap self, IDeleter& del)
  {
    if (self.isOriginal()) {
      MLOG_DETAIL(mlog::cap, "delete PageMap", self);
      for (size_t i = 0; i < num_caps(); ++i) {
        auto res = del.deleteEntry(_cap_table(i));
        ASSERT_MSG(res, "Mapped entries must be deletable.");
        if (!res) return res;
      }
      del.deleteObject(del_handle);
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
    auto capData = PageMapData().writable(entry.configurable);
    auto newCap = table.cap().asReference().withPtr(&mappedPageMapHelper).withData(capData);
    MLOG_DETAIL(mlog::cap, "mapTable", DVARhex(&tableEntry), DVARhex(pme), DVAR(index), DVAR(entry));
    cap::resetReference(_cap_table(index), [&]{ pme->reset(); });
    return cap::setReference(_cap_table(index), newCap, *tableEntry, table.cap(), [=,&entry]{ pme->set(entry); });
  }

  optional<Cap> PageMap::mint(CapEntry&, Cap self, CapRequest request, bool)
  {
    return PageMapData(self).mint(self, PageMapReq(request));
  }


  optional<void> PageMap::mapFrame(size_t index, CapEntry* frameEntry, MapFlags flags, size_t offset)
  {
    TypedCap<IFrame> frame(frameEntry);
    if (!frame) RETHROW(frame);
    auto frameInfo = frame.getFrameInfo();

    MLOG_INFO(mlog::cap, "mapFrame", DVAR(level()), DVAR(frame.cap()), DVAR(index), DVAR(offset),
                  DVARhex(frameInfo.start.physint()), DVARhex(frameInfo.size), DVARhex(pageSize()));
    ASSERT(index < num_caps());
    // check sizes and alignment
    uintptr_t frameaddr = frameInfo.start.physint() + offset;
    if (frameaddr % pageSize() != 0) THROW(Error::PAGEMAP_MISSING);
    if (offset + pageSize() > frameInfo.size) THROW(Error::INSUFFICIENT_RESOURCES);
    MLOG_DETAIL(mlog::cap, "mapFrame checks done");

    // build entry: page flag only valid on PML2 and higher
    /// @todo what about W^X ?
    /// @todo what about write_combine? (PAT/PAT2)
    auto pme = &_pm_table(index);
    auto entry = PageTableEntry().present(true).userMode(true).page(level() > 1)
      .writeable(flags.writable && frameInfo.writable)
      //.executeDisabled(!flags.executable) // not working on KNC???
      .writeThrough(flags.write_through)
      .cacheDisabled(flags.cache_disabled)
      .withAddr(frameaddr)
      .configurable(frameInfo.writable)
      .pmPtr(pme->pmPtr);

    // inherit
    auto newCap = frame.cap().asReference().withPtr(&mappedFrameHelper);
    MLOG_DETAIL(mlog::cap, "mapFrame", DVAR(&frameEntry), DVARhex(pme), DVAR(index), DVAR(entry),
      DVARhex(frameInfo.size), DVARhex(frameInfo.start.physint()));
    cap::resetReference(_cap_table(index), [&]{ pme->reset(); });
    return cap::setReference(_cap_table(index), newCap, *frameEntry, frame.cap(), [=,&entry]{ pme->set(entry); });
  }

  optional<void> PageMap::unmapEntry(size_t index)
  {
    MLOG_INFO(mlog::cap, "unmapEntry", DVAR(level()), DVAR(index));
    ASSERT(index < num_caps());
    auto pme = &_pm_table(index);
    cap::resetReference(_cap_table(index), [&]{ pme->reset(); }); // ignore lost races
    RETURN(Error::SUCCESS);
  }

  optional<void> PageMap::ProtectPageVisitor::applyPage(PageTableEntry* table, size_t index)
  {
    MLOG_INFO(mlog::cap, "mprotect", DVAR(index), DVARhex(flags.value));
    PageTableEntry pme = table[index].load(); // atomic load of the entry for later compare_exchange!
    // no assert here because the table could have been modified since the previous check!
    if (!pme.present || !pme.page) THROW(Error::LOST_RACE);
    auto entry = pme.writeable(flags.writable && pme.configurable)
      //.executeDisabled(!req.executable) // TODO not working on KNC
      .writeThrough(flags.write_through)
      .cacheDisabled(flags.cache_disabled);
    if (!table[index].replace(pme, entry)) THROW(Error::LOST_RACE); // TODO or simply ignore the lost race?
    RETURN(Error::SUCCESS);
  }

  optional<void> PageMap::visitPages(PageTableEntry* table, size_t LEVEL, PageOp& op)
  {
    size_t tableStart = op.table_vaddr;
    size_t startIndex = (op.vaddr()-op.table_vaddr)/pageSize(LEVEL);
    if (startIndex >= num_caps(LEVEL)) THROW(Error::REQUEST_DENIED);
    // calculate the number of entries by rounding up the end of the range and then substract the rounded down start.
    size_t numEntries = (op.vaddr()+op.sizeRemaining()+pageSize(LEVEL)-1)/pageSize(LEVEL) - op.vaddr()/pageSize(LEVEL);
    if (numEntries > num_caps(LEVEL)-startIndex) numEntries = num_caps(LEVEL)-startIndex; // restrict range to this table
    for (size_t i=0; i < numEntries; i++) {
      op.current_level = LEVEL;
      auto pme = table[startIndex+i].load(); // atomic copy
      MLOG_DETAIL(mlog::cap, "operateFrame", DVARhex(op.vaddr()), DVARhex(op.sizeRemaining()),
                  DVARhex(op.offset()), DVAR(op.current_level));
      if (LEVEL > 1 && pme.present && !pme.page) { // recurse into lower page map
        if (!pme.configurable) THROW(Error::PAGEMAP_NOCONF); /// @todo or just skip?
        // The table pointed to by the pme cannot be deleted concurrently because of the delete synchronisation broadcast.
        // Thus, we arrive at a valid page table in the recursive call.
        op.table_vaddr = tableStart + (startIndex+i)*pageSize(LEVEL);
        auto res = visitPages(phys2kernel<PageTableEntry>(pme.getAddr()), LEVEL-1, op); // recursive descent
        if (!res) RETHROW(res);
        // don't do moveForward() because it was done by LEVEL-1
      } else if (pme.present && pme.page) { // apply frame operator on mapped page
        if (LEVEL >= 2) { // check if the operation is aligned to the complete page, otherwise a pagemap is needed
          if (op.vaddr() % pageSize(LEVEL) != 0) THROW(Error::PAGEMAP_MISSING); // TODO should be more specific error
          if (op.sizeRemaining() < pageSize(LEVEL)) THROW(Error::PAGEMAP_MISSING);
        } else if (LEVEL == 1) { // check alignment
          if (op.vaddr() % pageSize(LEVEL) != 0) THROW(Error::UNALIGNED);
          if (op.sizeRemaining() < pageSize(LEVEL)) THROW(Error::UNALIGNED);
        }
        // the pme checks can become invalid by concurrent modifications but this is checked later again
        auto res = op.applyPage(table, startIndex+i); // map the frame in over the existing frame
        if (!res) RETHROW(res);
        op.moveForward(pageSize(LEVEL));
      } else if (!op.skip_nonmapped) { // nothing mapped here but should be
        // TODO handle 1GiB level 3 pages if supported by the hardware
        if (LEVEL > 2) THROW(Error::PAGEMAP_MISSING); // no pages possible in PML3 & PML4
        if (LEVEL == 2 || LEVEL == 3) { // check if the operation is aligned to the complete page, otherwise a pagemap is needed
          if (op.vaddr() % pageSize(LEVEL) != 0) THROW(Error::PAGEMAP_MISSING);
          if (op.sizeRemaining() < pageSize(LEVEL)) THROW(Error::PAGEMAP_MISSING);
        } else if (LEVEL == 1) { // check alignment
          if (op.vaddr() % pageSize(LEVEL) != 0) THROW(Error::UNALIGNED);
          if (op.sizeRemaining() < pageSize(LEVEL)) THROW(Error::UNALIGNED);
        }
        // the pme checks can become invalid by concurrent modifications but this is checked later again
        auto res = op.applyPage(table, startIndex+i); // map the frame in
        if (!res) RETHROW(res);
        op.moveForward(pageSize(LEVEL));
      } else { // nothing was mapped and we shall skip this entry
        op.moveForward(pageSize(LEVEL) - op.vaddr() % pageSize(LEVEL)); // move to the begin of the next entry
      }
    }
    RETURN(Error::SUCCESS);
  }


  optional<void> PageMap::visitTables(PageTableEntry* table, size_t LEVEL, TableOp& op)
  {
    size_t tableStart = op.table_vaddr;
    size_t index = (op.vaddr-op.table_vaddr)/pageSize(LEVEL);
    op.current_level = LEVEL;
    op.failaddr = (op.vaddr/pageSize(LEVEL))*pageSize(LEVEL);
    if (index >= num_caps(LEVEL)) THROW(Error::REQUEST_DENIED);
    auto pme = table[index].load();
    if (op.target_level == LEVEL) { // apply
      RETURN(op.applyTable(table2PageMap(table), index));
    } else if (LEVEL > 1 && pme.present && !pme.page) { // recurse into lower page map
      if (!pme.configurable) THROW(Error::PAGEMAP_NOCONF);
      op.table_vaddr = tableStart + index*pageSize(LEVEL);
      RETURN(visitTables(phys2kernel<PageTableEntry>(pme.getAddr()), LEVEL-1, op));
    } else THROW(Error::PAGEMAP_MISSING);
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

  optional<void> PageMap::mapFrame(uintptr_t vaddr, size_t size, CapEntry* frameEntry, MapFlags flags, uintptr_t offset,
                                   uintptr_t* failaddr, size_t* faillevel)
  {
    *failaddr = 0;
    *faillevel = 0;
    MapFrameVisitor op(vaddr, size, frameEntry, flags, offset);
    if (!op.frame) RETHROW(op.frame.state());
    auto res = visitPages(&_pm_table(0), level(), op);
    *failaddr = op.vaddr();
    *faillevel = op.current_level;
    RETURN(res.state());
  }

  Error PageMap::invokeMmap(Tasklet*, Cap self, IInvocation* msg)
  {
    PageMapData pd(self);
    if (!pd.writable) return Error::REQUEST_DENIED;
    auto data = msg->getMessage()->read<protocol::PageMap::Mmap>();
    uintptr_t failaddr;
    size_t faillevel;
    auto frameEntry = msg->lookupEntry(data.tgtFrame());
    if (!frameEntry) return Error::INVALID_CAPABILITY;
    auto res = this->mapFrame(data.vaddr, data.size, *frameEntry, data.flags, data.offset, &failaddr, &faillevel);
    msg->getMessage()->write<protocol::PageMap::Result>(failaddr, faillevel);
    return res.state();
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
    UnmapFrameVisitor op(data.vaddr, data.size);
    auto res = visitPages(&_pm_table(0), level(), op);
    msg->getMessage()->write<protocol::PageMap::Result>(op.vaddr(), op.current_level);
    return res.state();
  }

  Error PageMap::invokeMprotect(Tasklet*, Cap self, IInvocation* msg)
  {
    PageMapData pd(self);
    if (!pd.writable) return Error::REQUEST_DENIED;
    auto data = msg->getMessage()->read<protocol::PageMap::Mprotect>();
    ProtectPageVisitor op(data.vaddr, data.size, data.flags);
    auto res = visitPages(&_pm_table(0), level(), op);
    msg->getMessage()->write<protocol::PageMap::Result>(op.vaddr(), op.current_level);
    return res.state();
  }

  optional<void> PageMap::mapTable(uintptr_t vaddr, size_t target_level, CapEntry* tableEntry, MapFlags flags,
                                    uintptr_t* failaddr, size_t* faillevel)
  {
    *failaddr = 0;
    *faillevel = 0;
    MapTableVisitor op(vaddr, target_level, tableEntry, flags);
    if (op.target_level<1 || op.target_level>4 || op.vaddr % pageSize(op.target_level) != 0) THROW(Error::UNALIGNED);
    auto res = visitTables(&_pm_table(0), level(), op);
    *failaddr = op.failaddr;
    *faillevel = op.current_level;
    RETHROW(res.state());
  }

  Error PageMap::invokeInstallMap(Tasklet*, Cap self, IInvocation* msg)
  {
    PageMapData pd(self);
    if (!pd.writable) return Error::REQUEST_DENIED;
    auto data = msg->getMessage()->read<protocol::PageMap::InstallMap>();
    uintptr_t failaddr;
    size_t faillevel;
    auto tableEntry = msg->lookupEntry(data.pagemap());
    if (!tableEntry) return Error::INVALID_CAPABILITY;
    auto res = this->mapTable(data.vaddr, data.level, *tableEntry, data.flags, &failaddr, &faillevel);
    msg->getMessage()->write<protocol::PageMap::Result>(failaddr, faillevel);
    return res.state();
  }

  Error PageMap::invokeRemoveMap(Tasklet*, Cap self, IInvocation* msg)
  {
    PageMapData pd(self);
    if (!pd.writable) return Error::REQUEST_DENIED;
    auto data = msg->getMessage()->read<protocol::PageMap::RemoveMap>();
    UnmapTableVisitor op(data.vaddr, data.level);
    if (op.target_level<1 || op.target_level>4 || op.vaddr % pageSize(op.target_level) != 0) {
      msg->getMessage()->write<protocol::PageMap::Result>(op.failaddr, op.current_level);
      return Error::UNALIGNED;
    }
    auto res = visitTables(&_pm_table(0), level(), op);
    msg->getMessage()->write<protocol::PageMap::Result>(op.failaddr, op.current_level);
    return res.state();
  }

  optional<PageMap*>
  PageMapFactory::factory(CapEntry* dstEntry, CapEntry* memEntry, Cap memCap, IAllocator* mem,
                         size_t level)
  {
    if (!(level>=1 && level<=4)) THROW(Error::INVALID_ARGUMENT);
    auto table = mem->alloc(FrameSize::PAGE_MIN_SIZE, FrameSize::PAGE_MIN_SIZE);
    if (!table) {
      dstEntry->reset();
      RETHROW(table);
    }
    auto caps = mem->alloc(sizeof(CapEntry)*PageMap::num_caps(level), FrameSize::PAGE_MIN_SIZE);
    if (!caps) {
      mem->free(*table, FrameSize::PAGE_MIN_SIZE);
      dstEntry->reset();
      RETHROW(caps);
    }
    auto obj = mem->create<PageMap>(level, *table, *caps);
    if (!obj) {
      mem->free(*table, FrameSize::PAGE_MIN_SIZE);
      mem->free(*caps, sizeof(CapEntry)*PageMap::num_caps(level));
      dstEntry->reset();
      RETHROW(obj);
    }
    auto cap = Cap(*obj).withData(PageMapData());
    auto res = cap::inherit(*memEntry, memCap, *dstEntry, cap);
    if (!res) {
      mem->free(*obj); // mem->release(obj) goes throug IKernelObject deletion mechanism
      mem->free(*table, FrameSize::PAGE_MIN_SIZE);
      mem->free(*caps, sizeof(CapEntry)*PageMap::num_caps(level));
      RETHROW(res);
    }
    return *obj;
  }

} // namespace mythos
