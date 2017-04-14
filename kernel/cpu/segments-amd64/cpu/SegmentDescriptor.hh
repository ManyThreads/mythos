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
#include <cstddef>
#include <cstdint>

namespace mythos {

  /** The base class for all Segment Descriptors.
   *
   * @author Maik Krueger
   */
  class PACKED SegmentDescriptor
  {
  public:
    enum PrivilegeLevel {
      SYSTEM = 0x0,
      USER   = 0x3
    };
    enum Type {
      DATA_RO       = 0x10,
      DATA_RW       = 0x12,
      DATA_RWA      = 0x13,
      DATA_RWAE     = 0x17,

      CODE_E        = 0x18,
      CODE_EA       = 0x19,
      CODE_ER       = 0x1A,
      CODE_ERA      = 0x1B,
      CODE_EC       = 0x1C,
      CODE_ECA      = 0x1D,
      CODE_ERC      = 0x1E,
      CODE_ERCA     = 0x1F,

      SYS_LDT       = 0x02,
      SYS_TASK_GATE = 0x05,
      SYS_TSS       = 0x09,
      SYS_TSS_BUSY  = 0x0B,
      SYS_CALL_GATE = 0x0C,
      SYS_INT_GATE  = 0x0E,
      SYS_TRAP_GATE = 0x0F
    };
    enum Granularity
    {
      BYTES = 0u,
      PAGES = 1u
    };

    SegmentDescriptor() : present(0) {}

    void set(PrivilegeLevel priv, Type type, uint32_t base, uint32_t limit,
	     Granularity gran, bool db, bool code_64_bit, bool present) {
      dpl = priv & 0x3;
      this->type = type & 0x1F;
      granularity = gran & 1;
      this->db = db & 1;
      longmode = code_64_bit & 1;
      this->present = present & 1;
      setBaseAddress(base);
      setSegmentLimit(limit);
    }

    PrivilegeLevel getPrivilegeLevel() const { return PrivilegeLevel(dpl); }
    Granularity getGranularity() const { return Granularity(granularity); }
    bool getDB() const { return db; }
    bool getLongMode() const { return longmode; }
    bool getPresent() const { return present; }
    uint32_t getBaseAddress() const { return uint32_t(base0 | (base16<<16) | (base24<<24)); }
    uint32_t getSegmentLimit() const { return uint32_t(limit0 | (limit16<<16)); }
    Type getType() const { return Type(type); }
    void setType(Type type) { this->type = type & 0xF; }

    void setBaseAddress(uint32_t base) {
      base0 = base & 0xFFFF;
      base16 = uint8_t(base >> 16);
      base24 = uint8_t(base >> 24);
    }

    void setSegmentLimit(uint32_t limit) {
      limit0 = uint16_t(limit);
      limit16 = (limit >> 16) & 0xF;
    }

  protected:
    uint32_t limit0:16, base0:16;
    uint16_t base16:8, type:5, dpl:2, present:1;
    uint16_t limit16:4, avl:1, longmode:1, db:1, granularity:1, base24:8;
  };

  struct PACKED PseudoDescriptor
  {
    PseudoDescriptor(uint16_t l, void* b) : limit(l), base(b) {}
    uint16_t limit;
    void* base;
  };
  
  template<class SEGMENTS>
  inline void load_gdt(SEGMENTS& gdt)
  {
    PseudoDescriptor desc(uint16_t(sizeof(SEGMENTS) - 1), &gdt);
    asm volatile ("lgdt %0" : : "m" (desc));
  }

} // namespace mythos
