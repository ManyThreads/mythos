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
 * Copyright 2014 Randolf Rotta, Maik Krüger, and contributors, BTU Cottbus-Senftenberg 
 */
#pragma once

#include "util/compiler.hh"
#include "cpu/SegmentDescriptor.hh"
#include <cstdint>

namespace mythos {

  /** plain old data object for the Task State Segment, 64bit x86 variant. */
  // TODO add alignment constraint
  class PACKED TSS64
  {
  public:
    enum Constants {
      IO_MAP_SIZE = 0xFFFF,
      IO_MAP_OFFSET = 104,
      IO_MAP_OFFSET_INVALID = 104 + 16
    };

    TSS64()
      : ioMapOffset(IO_MAP_OFFSET_INVALID) {}

  public:
    uint32_t link; //< reserved on AMD64
    uint64_t sp[3]; //< stack pointer for rings 0--2, used by call/interrupt gate on CPL change
    uint64_t ist[8]; //< stack pointers for interrupt handlers, ist[0] is reserved
    uint64_t reserved;
    uint16_t debugTrap; //< raise debug exception on entry if bit 0 is set
    uint16_t ioMapOffset; //< offset to the IO permissions bitmap
    //uint8_t ioMap[IO_MAP_SIZE / 8];
    //uint8_t ioMapEnd;
  };

  class PACKED TSS64Descriptor
    : public SegmentDescriptor
  {
  public:
    TSS64Descriptor() : base32(0), reserved(0) {}

    void load(uint16_t sel) const { asm volatile ("ltr %w0" : : "rm" (sel)); }
    void setUnbusy() { setType(SegmentDescriptor::SYS_TSS); }

    void set(SegmentDescriptor::PrivilegeLevel priv, uint64_t base, uint32_t limit,
	     SegmentDescriptor::Granularity gran, bool present) {
      SegmentDescriptor::set(priv, SegmentDescriptor::SYS_TSS, uint32_t(base & 0xFFFFFFFF), limit,
			     gran, false, false, present);
      this->base32 = uint32_t(base >> 32);
    }

    template<class T>
    void set(T* ptr) {
      setBaseAddress(reinterpret_cast<uint64_t>(ptr) & 0xFFFFFFFF);
      this->base32 = uint32_t(reinterpret_cast<uint64_t>(ptr) >> 32);
      setSegmentLimit(sizeof(T));
    }

  protected:
    uint32_t base32;
    uint32_t reserved;
  };
  
} // namespace mythos
