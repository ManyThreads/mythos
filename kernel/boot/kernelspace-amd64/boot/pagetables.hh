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

#include <cstdint>

#include "util/compiler.hh"
#include "boot/memory-layout.h"

namespace mythos {
namespace boot {
  constexpr uint64_t INVALID  = 0;
  constexpr uint64_t PRESENT  = 1<<0;
  constexpr uint64_t WRITE    = 1<<1;
  constexpr uint64_t USER     = 1<<2;
  constexpr uint64_t PWT      = 1<<3;
  constexpr uint64_t CD       = 1<<4;
  constexpr uint64_t ACCESSED = 1<<5;
  constexpr uint64_t DIRTY    = 1<<6;
  constexpr uint64_t ISPAGE   = 1<<7;
  constexpr uint64_t GLOBAL   = 1<<8;
  //constexpr uint64_t GLOBAL   = 0;
  //constexpr uint64_t EXECUTEDISABLE = 1ull<<63;
  constexpr uint64_t EXECUTEDISABLE = 0;

  constexpr uint64_t PML2_PAGESIZE  = 1ull<<21;
  constexpr uint64_t PAGETABLE_SIZE = 4096;

  ALIGN_4K extern uint64_t devices_pml1[];
  ALIGN_4K extern uint64_t devices_pml2[];
  ALIGN_4K extern uint64_t image_pml2[];
  ALIGN_4K extern uint64_t pml2_tables[];
  ALIGN_4K extern uint64_t pml3_table[];
  ALIGN_4K extern uint64_t pml4_table[] SYMBOL("BOOT_PML4");

  inline uint64_t table_to_phys_addr(uint64_t* t, uint64_t subtable) {
    return reinterpret_cast<uint64_t>(t) + subtable*PAGETABLE_SIZE - VIRT_ADDR;
  }

  constexpr uint64_t PML1_BASE = PRESENT + WRITE + ACCESSED + DIRTY + GLOBAL;
  constexpr uint64_t PML2_BASE = PRESENT + WRITE + ACCESSED + DIRTY + ISPAGE + GLOBAL;
  constexpr uint64_t PML3_BASE = PRESENT + WRITE + USER + ACCESSED;
  constexpr uint64_t PML4_BASE = PRESENT + WRITE + USER + ACCESSED;

} // boot
} // mythos
