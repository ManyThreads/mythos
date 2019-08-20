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

#include "cpu/SegmentDescriptor.hh"
#include "cpu/tss64.hh"

namespace mythos {

  /** Standard Global Descriptor Table on x86-64 processors.
   *
   * The layout of the kernel and user code and data segments is
   * predeterminded by the requirements of the x86-64 syscall
   * instruction. User FS and GS might be unused because the base
   * address can be set via MSRs. The kernel TSS is needed for
   * interrupt entry.
   *
   * @warning If changing the layout, also check gdt-layout.h and start.S !!!
   */ 
  struct PACKED GdtAmd64
  {
    SegmentDescriptor unused0;     // 0x00
    SegmentDescriptor unused1;     // 0x08
    SegmentDescriptor kernel_code; // 0x10
    SegmentDescriptor kernel_data; // 0x18
    SegmentDescriptor user_code32; // 0x20
    SegmentDescriptor user_data;   // 0x28
    SegmentDescriptor user_code;   // 0x30
    SegmentDescriptor kernel_fs;   // 0x38
    SegmentDescriptor kernel_gs;   // 0x40
    TSS64Descriptor tss_kernel;    // 0x48 and 0x50

    void tss_kernel_load() const { tss_kernel.load(0x48); }
    void tss_kernel_unbusy() { tss_kernel.setUnbusy(); tss_kernel_load(); }

    void init() {
      kernel_code.set(SegmentDescriptor::SYSTEM, SegmentDescriptor::CODE_ERA,
		      0, 0xFFFFF, SegmentDescriptor::PAGES, false, true, true);
      kernel_data.set(SegmentDescriptor::SYSTEM, SegmentDescriptor::DATA_RWA,
		      0, 0xFFFFF, SegmentDescriptor::PAGES, true, true, true);
      // TODO user_code32 is uninitialised
      user_code.set(SegmentDescriptor::USER, SegmentDescriptor::CODE_ERA,
		    0, 0xFFFFF, SegmentDescriptor::PAGES, false, true, true);
      user_data.set(SegmentDescriptor::USER, SegmentDescriptor::DATA_RWA,
		    0, 0xFFFFF, SegmentDescriptor::PAGES, false, true, true);
      kernel_fs.set(SegmentDescriptor::USER, SegmentDescriptor::DATA_RWA,
		     0, 0xFFFFF, SegmentDescriptor::PAGES, true, true, true);
      kernel_gs.set(SegmentDescriptor::USER, SegmentDescriptor::DATA_RWA,
		    0, 0xFFFFF, SegmentDescriptor::PAGES, true, true, true);
      tss_kernel.set(SegmentDescriptor::USER, 0, 0, SegmentDescriptor::BYTES, true);
    }
    
    void load() {
	  load_gdt(*this);
	  asm volatile("mov %0,%%ss" : : "rm" (uint16_t(0x18))); // SS
	  asm volatile("mov %0,%%ds" : : "rm" (uint16_t(0x28))); // DS
	  asm volatile("mov %0,%%es" : : "rm" (uint16_t(0x28))); // ES
	  asm volatile("mov %0,%%fs" : : "rm" (uint16_t(0x38))); // FS
	  asm volatile("mov %0,%%gs" : : "rm" (uint16_t(0x40))); // GS
    }

  };
} // namespace mythos
