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

#include "util/optional.hh"
#include "objects/Cap.hh"
#include "objects/IFrame.hh"
#include "objects/IPageMap.hh"
#include "mythos/protocol/Frame.hh"
#include "mythos/protocol/PageMap.hh"
#include "objects/mlog.hh"

namespace mythos {

  /** @todo move to protocol because this is shared with the user-mode libraries */ 
  struct FrameSize {
    static constexpr size_t PAGE_MIN_SIZE = 1ull << 12; // 4096 bytes
    static constexpr size_t PAGE_SIZE_SHIFT = 9; // 9 bits per page map level
    static constexpr size_t PAGE_SIZE_FACTOR = 1ull << PAGE_SIZE_SHIFT; // 9 bits per page map level
    static constexpr size_t FRAME_MIN_SIZE = PAGE_MIN_SIZE; // 4096 bytes
    static constexpr size_t FRAME_SIZE_SHIFT = 1; // 1 bit increment per mythos frame, ie. all powers of two
    static constexpr size_t FRAME_SIZE_FACTOR = 1ull << FRAME_SIZE_SHIFT; // double the size
    static constexpr size_t FRAME_MAX_BITS = 25; // at lest 32-12=20 bits, at most what still fits into the capability data

    static constexpr size_t REGION_MAX_SIZE = FRAME_MIN_SIZE * (1ull << FRAME_MAX_BITS); // 25bits for offset = 128GiB
    // use 2048 static memory regions to cover the whole 48 bits physical address space with 25+12 bits per region
    // this requires 80KiB of memory with 40 bytes per region, but we are working on making it smaller :)
    static constexpr size_t STATIC_MEMORY_REGIONS = (1ull<<48)/REGION_MAX_SIZE;

    constexpr static size_t logBase(size_t n, size_t base) {
      return ( (n<base) ? 0 : 1+logBase(n/base, base));
    }

    constexpr static size_t frameBits2Size(size_t bits) {
      return FRAME_MIN_SIZE << (bits*FRAME_SIZE_SHIFT);
    }

    constexpr static size_t frameSize2Bits(size_t size) {
      return logBase(size/FRAME_MIN_SIZE, FRAME_SIZE_FACTOR);
    }

    constexpr static size_t pageLevel2Size(size_t level) {
      return PAGE_MIN_SIZE << (level*PAGE_SIZE_SHIFT);
    }

    constexpr static size_t pageSize2Level(size_t size) {
      return logBase(size/PAGE_MIN_SIZE, PAGE_SIZE_FACTOR);
    }
  };

  /** capability data for lightweight frames and for mapped frames. */
  BITFIELD_DEF(CapData, FrameData)
  typedef protocol::Frame::FrameReq FrameReq;
  BoolField<value_t, base_t, 0> writable; // can write to this memory
  BoolField<value_t, base_t, 1> kernel;   // can be accessed by the kernel (not device memory)
  UIntField<value_t, base_t, 2, 5> sizeBits;
  UIntField<value_t, base_t, 7, 25> offset; // start in its memory region

  FrameData() : value(0) {} /** @todo why writable by default here? */
  FrameData(Cap cap) : value(cap.data()) {}

  size_t getSize() const { return FrameSize::frameBits2Size(sizeBits); }
  FrameData size(size_t size) const {
    auto bits = FrameSize::frameSize2Bits(size);
    ASSERT(size == FrameSize::frameBits2Size(bits));
    return this->sizeBits(bits);
  }
  //void setSize(size_t size) { sizeIndex = FrameSize::frameSize2Bits(size); }
  uintptr_t getStart(uintptr_t base) const { return base + offset*FrameSize::FRAME_MIN_SIZE; }
  uintptr_t getEnd(uintptr_t base) const { return getStart(base)+getSize(); }
  FrameData start(uintptr_t base, uintptr_t addr) {
    ASSERT(base <= addr && addr < base+FrameSize::REGION_MAX_SIZE);
    return this->offset((addr-base)/FrameSize::FRAME_MIN_SIZE); 
  }
  //bool isAligned() const { return (getStart(0)/getSize())*getSize() == getStart(0); }

  /** make a capability that restricts the frame to a subrange with possibly reduced access rights.
   * The operation fails if the requested subrange is not inside the current range as described by the capability.
   * It also fails if a privilege escalation is requested.
   */
  static optional<Cap> subRegion(Cap self, uintptr_t base, size_t size, FrameReq r, bool derive) {
    auto old = FrameData(self);
    auto oldRange = Range<uintptr_t>::bySize(old.getStart(base), self.isOriginal() ? size : old.getSize());
    auto newRange = Range<uintptr_t>::bySize(old.getStart(base) + FrameSize::FRAME_MIN_SIZE*r.offset, FrameSize::frameBits2Size(r.sizeBits));
    if (!oldRange.contains(newRange)) THROW(Error::INSUFFICIENT_RESOURCES);
    if (r.writable && !old.writable) THROW(Error::INSUFFICIENT_RESOURCES);
    if (r.kernel && !old.kernel) THROW(Error::INSUFFICIENT_RESOURCES);
    auto res = FrameData().writable(r.writable).kernel(r.kernel).start(base, newRange.getStart()).size(newRange.getSize());
    if (derive) return self.asDerived().withData(res);
    else return self.asReference().withData(res);
  }

  BITFIELD_END

  /** x86-64 page table entries.
   * 
   * Entries that reference page tables ignore bits 8--11 and 52--62 (not reserved by intel!).
   * We use bits 52--62 for a partial pointer to the table's PageMap object.
   * We use bit 8 for mapped tables to tell that recursive operations can modify the referenced table.
   * We use bit 8 for mapped frames to tell that the frame capability was writable.
   */
  BITFIELD_DEF(uint64_t, PageTableEntry)
  enum Config { MAXPHYADDR = 40 };
  BoolField<value_t, base_t, 0> present;
  BoolField<value_t, base_t, 1> writeable;
  BoolField<value_t, base_t, 2> userMode;
  BoolField<value_t, base_t, 3> writeThrough;
  BoolField<value_t, base_t, 4> cacheDisabled;
  BoolField<value_t, base_t, 5> access;
  BoolField<value_t, base_t, 6> dirty;
  BoolField<value_t, base_t, 7> page;
  BoolField<value_t, base_t, 8> global;
  BoolField<value_t, base_t, 12> pat2; // only for mapped 2MiB and 1GiB pages
  UIntField<value_t, base_t, 12, (MAXPHYADDR - 12)> addr;
  BoolField<value_t, base_t, 8> configurable; // MyThOS page table: can modify the mapped table, MYTHOS page: has write access rights
  UIntField<value_t, base_t, 52, 10> pmPtr; // MyThOS: table's partial IPageMap* in first 3 entries
  BoolField<value_t, base_t, 63> executeDisabled;
  PageTableEntry() : value(0) {}
  PageTableEntry(PageTableEntry const& v) : value(v.value) {}
  /// @todo add atomic variant for compare_exchange
  std::atomic<uint64_t> atomic;
  static_assert(sizeof(std::atomic<uint64_t>) == sizeof(uint64_t), "insufficient atomic op");
  void reset() { atomic = PageTableEntry().pmPtr(pmPtr); }
  void set(PageTableEntry v) { atomic = v.value; }
  PageTableEntry load() { return atomic.load(); }
  bool replace(PageTableEntry& oldVal, PageTableEntry newVal) {
    return atomic.compare_exchange_strong(oldVal.value, newVal.value);
  }
  base_t withAddr(uint64_t a) { return addr(a>>12); }
  uint64_t getAddr() const { return addr<<12;}
  BITFIELD_END

  template<class S>
  inline ostream_base<S>& operator<<(ostream_base<S>& o, const PageTableEntry& e)
  {
    o << "<page map entry:" << (void*)e.getAddr();
    if (e.present) o << " Pr";
    if (e.global) o << " G";
    if (e.writeable) o << " W";
    if (e.userMode) o << " Us";
    if (e.executeDisabled) o << " XD";
    if (e.writeThrough) o << " PWT";
    if (e.cacheDisabled) o << " PCD";
    if (e.page) o << " Pa/PAT";
    if (e.pat2) o << " PAT2";
    if (e.access) o << " A";
    if (e.dirty) o << " D";
    o << '>';
    return o;
  }

  BITFIELD_DEF(CapData, PageMapData)
  typedef protocol::PageMap::PageMapReq PageMapReq;
  /** For mapped and non-mapped PageMap: can be modified. */
  BoolField<value_t, base_t, 1> writable;
  PageMapData() : value(0) { writable=true; } /** @todo why writable by default here? */ 
  PageMapData(Cap cap) : value(cap.data()) {}

  Cap mint(Cap self, PageMapReq r) const {
    PageMapData res = this->writable(writable && r.configurable);
    return self.asReference().withData(res);
  }
  BITFIELD_END

} // namespace mythos
