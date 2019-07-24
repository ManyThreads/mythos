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

  /** capability data for leightweight frames.
   *
   * The capability's object pointer points to a memory region.
   * The capability data stores just a offset relative to the region's base address.
   * Frames can have any power of two size beginning at 4KiB.
   *
   * @todo currently also used for mapped frames but not actually needed
   * because all the necessary information is stored in the page table entry.
   * See the addressRange() methods in PageMapAmd64.
   * Hence, the pointer to the page map object can be stored in the capability data
   * instead of the few free bits in the page table entries. Accessing the respective
   * capability entry given the page table entry's address is simple pointer rounding,
   * because the page tables are 4KiB aligned.
   */
  BITFIELD_DEF(CapData, FrameData)
  typedef protocol::Frame::FrameReq FrameReq;
  BoolField<value_t, base_t, 0> writable; // can write to this memory
  BoolField<value_t, base_t, 1> device;   // device memory: cannot be accessed by the kernel
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
    if (!r.device && old.device) THROW(Error::INSUFFICIENT_RESOURCES);
    auto res = FrameData().writable(r.writable).device(r.device).start(base, newRange.getStart()).size(newRange.getSize());
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
  std::atomic<uint64_t> atomic; // atomic variant for compare-exchange
  PageTableEntry() : value(0) {}
  PageTableEntry(PageTableEntry const& v) : value(v.value) {}
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
