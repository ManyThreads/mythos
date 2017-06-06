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

#include <array>
#include "util/alignments.hh"
#include "objects/IPageMap.hh"
#include "objects/IFactory.hh"
#include "objects/FrameDataAmd64.hh"
#include "objects/IKernelObject.hh"
#include "objects/CapEntry.hh"
#include "async/NestedMonitorDelegating.hh"
#include "mythos/protocol/PageMap.hh"

namespace mythos {

class IFrame;
class MappedData;

class PageMap final
  : public IKernelObject
  , public IPageMap
  , protected IResult<void>
{
public:
  typedef IPageMap::MapFlags MapFlags;
  typedef protocol::PageMap::PageMapReq PageMapReq;
  static constexpr size_t TABLE_SIZE = 512;

  PageMap(IAsyncFree* mem, size_t level, void* pageMem, void* capMem);
  PageMap(const PageMap&) = delete;

  static size_t num_caps(size_t l) { return l<4 ? TABLE_SIZE : TABLE_SIZE/2; }
  size_t level() const { return _level; }
  bool isRootMap() const { return _level == 4; }
  static IPageMap* table2PageMap(PageTableEntry const* table);
  size_t num_caps() const { return num_caps(_level); }
  static size_t pageSize(size_t level) { return 4096ul << (9*(level-1)); }
  size_t pageSize() const { return pageSize(_level); }

public: // IPageMap interface
  Info getPageMapInfo(Cap self) const override {
    return {_level, PhysPtr<void>::fromKernel(&_pm_table(0)), PageMapData(self).writable};
  }

  void print() const override;

  struct FrameOp {
    FrameOp(uintptr_t vaddr, size_t vsize)
      : start_vaddr(vaddr), start_vsize(vsize), offs(0), level(0), skip_nonmapped(false) {}
    virtual optional<void> applyFrame(PageTableEntry* table, size_t index) = 0;
    uintptr_t start_vaddr;
    size_t start_vsize;
    size_t offs;
    size_t level; // for error reporting
    bool skip_nonmapped;
    uintptr_t vaddr() const { return start_vaddr+offs; }
    size_t vsize() const { return start_vsize-offs; }
    size_t offset() const { return offs; }
    void moveForward(size_t pagesize) { offs += pagesize; }
  };

  struct MmapOp : public FrameOp {
    MmapOp(uintptr_t vaddr, size_t vsize, optional<CapEntry*> frameEntry, MapFlags flags);
    optional<void> applyFrame(PageTableEntry* table, size_t index) override;
    IPageMap::FrameOp frameOp;
  };

  struct MunmapOp : public FrameOp {
    MunmapOp(uintptr_t vaddr, size_t vsize) : FrameOp(vaddr, vsize) {
      skip_nonmapped = true;
    }
    optional<void> applyFrame(PageTableEntry* table, size_t index) override;
  };

  struct MprotectOp : public FrameOp {
    MprotectOp(uintptr_t vaddr, size_t vsize, MapFlags flags)
      : FrameOp(vaddr, vsize), flags(flags) { skip_nonmapped = true; }
    optional<void> applyFrame(PageTableEntry* table, size_t index) override;
    MapFlags flags;
  };

  struct TableOp {
    TableOp(uintptr_t vaddr, size_t tgtLevel)
      : vaddr(vaddr), tgtLevel(tgtLevel), level(0) {}
    virtual optional<void> applyTable(IPageMap* map, size_t index) = 0;
    uintptr_t vaddr;
    size_t tgtLevel;
    size_t level; // for error reporting
  };

  struct InstallMapOp : public TableOp {
    InstallMapOp(uintptr_t vaddr, size_t tgtLevel, CapEntry* mapEntry, MapFlags flags)
      : TableOp(vaddr, tgtLevel), mapEntry(mapEntry), flags(flags) {}
    CapEntry* mapEntry;
    MapFlags flags;
    optional<void> applyTable(IPageMap* map, size_t index) override;
  };

  struct RemoveMapOp : public TableOp {
    RemoveMapOp(uintptr_t vaddr, size_t tgtLevel)
      : TableOp(vaddr, tgtLevel) {}
    optional<void> applyTable(IPageMap* map, size_t index) override;
  };

  template<size_t LEVEL>
  static optional<void> operateFrame(PageTableEntry* table, FrameOp& op);
  optional<void> operateFrame(FrameOp& op, InvocationBuf* msg);

  optional<void> mapFrame(size_t index, IPageMap::FrameOp const& op, size_t offset) override;
  optional<void> unmapEntry(size_t index) override;

  template<size_t LEVEL>
  static optional<void> operateTable(PageTableEntry* table, TableOp& op);
  optional<void> operateTable(TableOp& op, InvocationBuf* msg);

  optional<void> mapTable(CapEntry* table, MapFlags req, size_t index) override;

public:
  /** IResult<optional> interface, used for shootdown. */
  virtual void response(Tasklet*, optional<void>) override;

public: // IKernelObject interface
  Range<uintptr_t> addressRange(Cap self) override;
  optional<void const*> vcast(TypeId id) const override {
    if (id == typeId<IPageMap>()) return static_cast<IPageMap const*>(this);
    THROW(Error::TYPE_MISMATCH);
  }

  optional<void> deleteCap(Cap self, IDeleter& del) override;
  void deleteObject(Tasklet* t, IResult<void>* r) override;
  optional<Cap> mint(Cap self, CapRequest request, bool derive) override;

  void invoke(Tasklet* t, Cap self, IInvocation* msg) override;
  Error invokeMmap(Tasklet* t, Cap self, IInvocation* msg);
  Error invokeRemap(Tasklet* t, Cap self, IInvocation* msg);
  Error invokeMunmap(Tasklet* t, Cap self, IInvocation* msg);
  Error invokeMprotect(Tasklet* t, Cap self, IInvocation* msg);
  Error invokeInstallMap(Tasklet* t, Cap self, IInvocation* msg);
  Error invokeRemoveMap(Tasklet* t, Cap self, IInvocation* msg);

private:
  size_t _level;
  MemoryDescriptor _memDesc[3];

  PageTableEntry& _pm_table(size_t index) const {
    ASSERT(index < TABLE_SIZE);
    return *(reinterpret_cast<PageTableEntry*>(_memDesc[0].ptr)+index);
  }

  CapEntry& _cap_table(size_t index) const {
    ASSERT(index < num_caps());
    return *(reinterpret_cast<CapEntry*>(_memDesc[1].ptr)+index);
  }

  async::NestedMonitorDelegating monitor;
  IAsyncFree* _mem;
  IDeleter::handle_t del_handle = {this};

  IResult<void>* _deletionSink;
  optional<void> _Obj(IDeleter& del);

  optional<PageTableEntry> _lookup(uintptr_t vaddr, const PageTableEntry* table, size_t level) const;
};

  class PageMapFactory : public FactoryBase
  {
  public:
    static optional<PageMap*>
    factory(CapEntry* dstEntry, CapEntry* memEntry, Cap memCap, IAllocator* mem, size_t level);

    Error factory(CapEntry* dstEntry, CapEntry* memEntry, Cap memCap,
                  IAllocator* mem, IInvocation* msg) const override {
      auto data = msg->getMessage()->cast<protocol::PageMap::Create>();
      return factory(dstEntry, memEntry, memCap, mem, data->level).state();
    }
  };


} // namespace mythos
