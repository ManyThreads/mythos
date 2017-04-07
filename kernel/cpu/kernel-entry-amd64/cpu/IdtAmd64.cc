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

#include "cpu/IdtAmd64.hh"
#include "cpu/gdt-layout.h"
#include "boot/mlog.hh"

namespace mythos {

  /** Helper table with the addresses of the irq entry routines. */
  extern uint64_t interrupt_entry_table[256] SYMBOL("interrupt_entry_table");

  void IdtAmd64::initEarly() {
    for (size_t i=0; i<SIZE; i++)
      table[i].setKernelIRQ(interrupt_entry_table[i], SEGMENT_KERNEL_CS, 0);
  }

  void IdtAmd64::init() {
    MLOG_DETAIL(mlog::boot, "init IDT with entry handlers at", (void*)interrupt_entry_table[0]);
    initEarly();
    // mark interrupts that shall be executed on the nmi stack
    table[2].setIST(1); // NMI Non-Maskable Interrupt
    table[8].setIST(1); // DF Double Fault
    table[0xe].setIST(1); // Page Fault, because the kernel stack might be full
  }

} // namespace mythos
