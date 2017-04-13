/* -*- mode:C++; -*- */
/* MyThOS: The Many-Threads Operating System
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

  struct FrameSize {
    typedef uint8_t index_t;
    static constexpr size_t MIN_FRAME_SIZE = 1ull << 12;
    static constexpr size_t FRAME_SIZE_SHIFT = 9;
    static constexpr size_t FRAME_SIZE_FACTOR = 1ull << FRAME_SIZE_SHIFT;
    static constexpr size_t MAX_FRAME_CLASS = 3;
    static constexpr size_t MAX_REGION_SIZE = MIN_FRAME_SIZE * (1ull << 29); // 29bits for offset

    constexpr static size_t sizeFromIndex(size_t index) {
      return MIN_FRAME_SIZE << (index*FRAME_SIZE_SHIFT);
    }

    static index_t maxIndexFromSize(size_t size)
    {
      // if (size < MIN_FRAME_SIZE) return -1;
      // else size /= MIN_FRAME_SIZE;
      size /= MIN_FRAME_SIZE;
      /// @todo use __builtin_clz Returns the number of leading 0-bits in X, starting at the most significant bit position
      uint8_t result = 0;
      while (size && result < 3) {
        size >>= FRAME_SIZE_SHIFT;
        result++;
      };
      return result;
    }
  };

  BITFIELD_DEF(CapData, FrameData)
  typedef protocol::Frame::FrameReq FrameReq;
  BoolField<value_t, base_t, 0> writable;
  UIntField<value_t, base_t, 1, 2> sizeIndex;
  UIntField<value_t, base_t, 3, 29> offset;
  FrameData() : value(0) { writable = true; }
  FrameData(Cap cap) : value(cap.data()) {}
  size_t size() const { return FrameSize::sizeFromIndex(sizeIndex); }
  void setSize(size_t size) { sizeIndex = FrameSize::maxIndexFromSize(size); }
  uintptr_t addr(uintptr_t base) const { return base + offset*FrameSize::MIN_FRAME_SIZE; }
  uintptr_t end(uintptr_t base) const { return addr(base)+size(); }
  void setAddr(uintptr_t base, uintptr_t addr)
  { offset = (addr-base)/FrameSize::MIN_FRAME_SIZE; }
  bool isAligned() const { return (addr(0)/size())*size() == addr(0); }

  Cap referenceRegion(Cap self, FrameReq r) const {
    FrameData res = this->writable(writable && r.writable);
    return self.asReference().withData(res);
  }

  optional<Cap> deriveRegion(Cap self, IKernelObject& frame, FrameReq r, size_t maxSize) const {
    FrameData res = this->writable(writable && r.writable)
      .offset(r.offset).sizeIndex(r.size);
    if (res.end(0)>maxSize || !res.isAligned()) THROW(Error::UNALIGNED);
    return self.asDerived().withPtr(&frame).withData(res);
  }

  optional<Cap> referenceFrame(Cap self, FrameReq r) const {
    FrameData res = writable(writable && r.writable)
      .offset(r.offset).sizeIndex(r.size);
    if (res.addr(0)<addr(0) || res.end(0)>end(0) || !res.isAligned()) THROW(Error::INVALID_REQUEST);
    return self.asReference().withData(res);
  }
  BITFIELD_END

  // entries that reference page tables ignore bits 8--11 and 52--62 (not reserved by intel!).
  // we use bits 52--62 for a partial pointer to the table's PageMap object.
  // we use bit 8 for mapped tables to say that recursive operations can modify the referenced table
  // we use bit 8 for mapped frames to say that the frame capability was writable

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
  BoolField<value_t, base_t, 8> configurable; // MyThOS: can modify the mapped table (not page)
  UIntField<value_t, base_t, 52, 10> pmPtr; // MyThOS: table's partial IPageMap* in first 3 entries
  BoolField<value_t, base_t, 63> executeDisabled;
  PageTableEntry() : value(0) {}
  PageTableEntry(PageTableEntry& v) : value(v.value) {}
  /// @todo add atomic variant for compare_exchange
  std::atomic<uint64_t> atomic;
  static_assert(sizeof(std::atomic<uint64_t>) == sizeof(uint64_t), "insufficient atomic op");
  void reset() { atomic = PageTableEntry().pmPtr(pmPtr); }
  void set(PageTableEntry v) { atomic = v.value; }
  bool replace(PageTableEntry oldVal, PageTableEntry newVal) {
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
  BoolField<value_t, base_t, 0> mapped; // is reference cap mapped into the page table
  /** For mapped and non-mapped PageMap: can be modified, for mapped Frame: was writable */
  BoolField<value_t, base_t, 1> writable;
  UIntField<value_t, base_t, 2, 9> index; // entry in the table (if mapped)
  PageMapData() : value(0) { writable=true; }
  PageMapData(Cap cap) : value(cap.data()) {}

  Cap mint(Cap self, PageMapReq r) const {
    PageMapData res = this->writable(writable && r.configurable);
    return self.asReference().withData(res);
  }

  template<class FI>
  static Cap mapFrame(IKernelObject* map, Cap frame, FI const& frameInfo, size_t index) {
    auto res = PageMapData(0).mapped(true).writable(frameInfo.writable).index(index);
    return frame.asReference().withPtr(map).withData(res);
  }

  static Cap mapTable(IKernelObject* map, Cap table, bool conf, size_t index) {
    auto res = PageMapData(0).mapped(true).writable(conf).index(index);
    return table.asReference().withPtr(map).withData(res);
  }
  BITFIELD_END

} // namespace mythos
