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
 * Copyright 2017 Randolf Rotta, Robert Kuban, and contributors, BTU Cottbus-Senftenberg
 */

#include "cpu/IOApic.hh"

namespace mythos {
  IOApic ioapic;

  void IOApic::init() {
    IOAPIC_VERSION ver(read(IOApic::IOAPICVER));
    MLOG_INFO(mlog::boot, "IOAPIC", DVAR(ver.version), DVAR(ver.max_redirection_table));

    RED_TABLE_ENTRY rte_irq;
    rte_irq.trigger_mode = 0;
    rte_irq.intpol = 0;
    rte_irq.dest = 0;
    rte_irq.int_mask = 0;

    RED_TABLE_ENTRY rte_pci; // interrupts 16..19
    rte_pci.trigger_mode = 1; // level triggered
    rte_pci.intpol = 1; // low active
    rte_pci.dest = 0;
    rte_pci.int_mask = 0;

    for (size_t i = 0; i < 16; i++) {
      rte_irq.intvec = BASE_IRQ + i;
      rte_irq.int_mask = i == 2 ? 1 : 0; // disable interrupt #2
      writeTableEntry(i, rte_irq);
    }

    for (size_t i = 16; i < ver.max_redirection_table + 1; i++) {
      rte_pci.intvec = BASE_IRQ + i;
      writeTableEntry(i, rte_pci);
    }
  }


  uint32_t IOApic::read(size_t reg) {
      base_address[0] = (reg & 0xff);
      return base_address[4];
  }

  void IOApic::write(size_t reg, uint32_t value) {
      base_address[0] = (reg & 0xff);
      base_address[4] = value;
  }

  IOApic::RED_TABLE_ENTRY IOApic::readTableEntry(size_t table_entry) {
    IOApic::RED_TABLE_ENTRY rte;
    rte.upper(read(IOApic::IOREDTBL_BASE+2*table_entry+1));
    rte.lower(read(IOApic::IOREDTBL_BASE+2*table_entry));
    return rte;
  }

  void IOApic::writeTableEntry(size_t table_entry, RED_TABLE_ENTRY rte) {
    write(IOApic::IOREDTBL_BASE+2*table_entry+1, (uint32_t)rte.upper); // always write upper first
    write(IOApic::IOREDTBL_BASE+2*table_entry, (uint32_t)rte.lower);
  }

  /**
   * Assumes a continuous mapping of interrupt vectors to redirection table entries starting at BASE_IRQ.
   * In other cases the used mapping has to be remembered or all redirection entries have to be searched through.
   */
  void IOApic::maskIRQ(uint64_t irq) {
    ASSERT(irq >= BASE_IRQ && irq < 256);
    auto table_entry = irq - BASE_IRQ;
    IOApic::RED_TABLE_ENTRY rte = readTableEntry(table_entry);
    rte.int_mask = 1;
    writeTableEntry(table_entry, rte);
  }

  /// Same as maskIRQ
  void IOApic::unmaskIRQ(uint64_t irq) {
    ASSERT(irq >= BASE_IRQ && irq < 256);
    auto table_entry = irq - BASE_IRQ;
    IOApic::RED_TABLE_ENTRY rte = readTableEntry(table_entry);
    rte.int_mask = 0;
    writeTableEntry(table_entry, rte);
  }

} // namespace mythos

