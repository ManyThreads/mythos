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
 * Copyright 2014 Randolf Rotta, Maik Krüger, and contributors, BTU Cottbus-Senftenberg 
 */
#pragma once

#include "util/PhysPtr.hh"
#include <cstdint> // uint64_t

namespace mythos {
        
  class E820Info
  {
  public:
    // table addr references the "zero page" addr of x86 linux kernel
    E820Info(size_t baseAddr = 0x90200)
      : elemCountAddr(baseAddr+0x1E8) // references to the kernel.org zero-page txt
      , memoryMap(baseAddr+0x2d0)
    { }

    enum Type {
      E_USABLE=1,
      E_RESERVED,
      E_ACPI_RECLAIMABLE,
      E_ACPI_NVS,
      E_BAD
    };
    
    struct PACKED Entry {
      PhysPtr<void> addr;
      uint64_t size;    
      uint32_t type;
      bool isUsable() const { return type == E_USABLE; }
    }; 
    
    Entry const& operator[](size_t idx) const { return *(memoryMap+idx); }
    size_t size() const { return *elemCountAddr; }

  protected: 
    PhysPtr<uint8_t> elemCountAddr;
    PhysPtr<Entry> memoryMap;
  }; //class E820Info

} // namespace mythos
