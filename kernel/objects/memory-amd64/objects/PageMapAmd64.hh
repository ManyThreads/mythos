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

  optional<void> mapFrame(uintptr_t vaddr, size_t size, CapEntry* frameEntry, MapFlags flags, uintptr_t offset, 
                                   uintptr_t* failaddr, size_t* faillevel) override; 
  optional<void> mapTable(uintptr_t vaddr, size_t level, CapEntry* tableEntry, MapFlags flags,
                                    uintptr_t* failaddr, size_t* faillevel) override;
  optional<void> mapFrame(size_t index, CapEntry* frameEntry, MapFlags flags, size_t offset) override;
  optional<void> unmapEntry(size_t index) override;
  optional<void> mapTable(CapEntry* table, MapFlags req, size_t index) override;


  /** visitor state for operations on pages: mapFrame, unmapFrame, protectPage. */ 
  struct PageOp {
    PageOp(uintptr_t vaddr, size_t size, size_t frame_offset, bool skip_nonmapped=false)
      : start_vaddr(vaddr), start_size(size), frame_offset(frame_offset), skip_nonmapped(skip_nonmapped) {}
    /** the operation to perform on every page in the target range from start_vaddr to start_vaddr+start_size.
     * TODO pass the page map entry value
     */
    virtual optional<void> applyPage(PageTableEntry* table, size_t index) = 0;
    const uintptr_t start_vaddr; //< start address in the logical address space
    const size_t start_size;
    const size_t frame_offset; //< start offset in the physical frame
    const bool skip_nonmapped; //< true if non-present pages should be ignored
    size_t table_vaddr = 0; //< start address of the currently visited page map 
    size_t page_offset = 0; //< current offset in virtual range, advances during the walk
    size_t current_level = 0; //< current level in the page map tree, for error reporting
    uintptr_t vaddr() const { return start_vaddr+page_offset; }
    size_t sizeRemaining() const { return start_size-page_offset; }
    size_t offset() const { return frame_offset+page_offset; }
    void moveForward(size_t pagesize) { page_offset += pagesize; }
  };

  /** entry point for the table walk for operations on pages. */
  optional<void> visitPages(PageOp& op);

  /** recursion through the page map tree, apply the operation on all pages in the target range. */
  static optional<void> visitPages(PageTableEntry* table, size_t LEVEL, PageOp& op);

  struct MapFrameVisitor : public PageOp {
    MapFrameVisitor(uintptr_t vaddr, size_t size, CapEntry* frameEntry, MapFlags flags, size_t offset)
      : PageOp(vaddr, size, offset, false), flags(flags), frameEntry(frameEntry), frame(frameEntry) {}
    optional<void> applyPage(PageTableEntry* table, size_t index) override {
      return table2PageMap(table)->mapFrame(index, frameEntry, flags, offset());
    }
    MapFlags flags;
    CapEntry* frameEntry;
    TypedCap<IFrame> frame;
  };

  struct UnmapFrameVisitor : public PageOp {
    UnmapFrameVisitor(uintptr_t vaddr, size_t size) : PageOp(vaddr, size, 0, true) {}
    optional<void> applyPage(PageTableEntry* table, size_t index) override { 
      return table2PageMap(table)->unmapEntry(index);
    }
  };

  struct ProtectPageVisitor : public PageOp {
    ProtectPageVisitor(uintptr_t vaddr, size_t size, MapFlags flags) : PageOp(vaddr, size, 0, true), flags(flags) {}
    optional<void> applyPage(PageTableEntry* table, size_t index) override;
    MapFlags flags;
  };

  
  
  /** visitor state for operations on tables (page maps): mapTable, unmapTable. */ 
  struct TableOp {
    TableOp(uintptr_t vaddr, size_t target_level)
      : vaddr(vaddr), target_level(target_level) {}
    /** the operation to perform on the target page map at the selected logical address and target level.
     * TODO pass the page map entry value
     */
    virtual optional<void> applyTable(IPageMap* map, size_t index) = 0;
    const uintptr_t vaddr;
    const size_t target_level;
    size_t table_vaddr = 0; //< start address of the currently visited page map 
    uintptr_t failaddr = 0; // for error reporting
    size_t current_level = 0; // for error reporting
  };

  /** recursion through the page map tree, apply the operation on the target page table. */
  static optional<void> visitTables(PageTableEntry* table, size_t LEVEL, TableOp& op);

  struct MapTableVisitor : public TableOp {
    MapTableVisitor(uintptr_t vaddr, size_t target_level, CapEntry* mapEntry, MapFlags flags)
      : TableOp(vaddr, target_level), mapEntry(mapEntry), flags(flags) {}
    CapEntry* mapEntry;
    MapFlags flags;
    optional<void> applyTable(IPageMap* map, size_t index) override { return map->mapTable(mapEntry, flags, index); }
  };

  struct UnmapTableVisitor : public TableOp {
    UnmapTableVisitor(uintptr_t vaddr, size_t target_level)
      : TableOp(vaddr, target_level) {}
    optional<void> applyTable(IPageMap* map, size_t index) override { return map->unmapEntry(index); }
  };

  
public:
  /** IResult<optional> interface, used for shootdown. */
  virtual void response(Tasklet*, optional<void>) override;

public: // IKernelObject interface
  Range<uintptr_t> addressRange(CapEntry&, Cap self) override;
  optional<void const*> vcast(TypeId id) const override {
    if (id == typeId<IPageMap>()) return static_cast<IPageMap const*>(this);
    THROW(Error::TYPE_MISMATCH);
  }

  optional<void> deleteCap(CapEntry&, Cap self, IDeleter& del) override;
  void deleteObject(Tasklet* t, IResult<void>* r) override;
  optional<Cap> mint(CapEntry&, Cap self, CapRequest request, bool derive) override;

  void invoke(Tasklet* t, Cap self, IInvocation* msg) override;
  Error invokeMmap(Tasklet* t, Cap self, IInvocation* msg);
  Error invokeRemap(Tasklet* t, Cap self, IInvocation* msg);
  Error invokeMunmap(Tasklet* t, Cap self, IInvocation* msg);
  Error invokeMprotect(Tasklet* t, Cap self, IInvocation* msg);
  Error invokeInstallMap(Tasklet* t, Cap self, IInvocation* msg);
  Error invokeRemoveMap(Tasklet* t, Cap self, IInvocation* msg);

private:
  class MappedFrame : public IKernelObject {
  public:
    MappedFrame(PageMap* map) : map(map) {}
    ~MappedFrame() override {}

    Range<uintptr_t> addressRange(CapEntry& entry, Cap self) override;
    optional<void> deleteCap(CapEntry&, Cap self, IDeleter& del) override;

    PageMap* const map;
  };

  MappedFrame mappedFrameHelper = {this};

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

  // not used anymore?
  // optional<PageTableEntry> _lookup(uintptr_t vaddr, const PageTableEntry* table, size_t level) const;
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
