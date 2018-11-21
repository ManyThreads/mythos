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
#include "cpu/sbox.hh"

namespace mythos {
  IOApic ioapic;

  /*
   * +        * 0    DMA Completion Interrupt for Channel 0 NOT USED
   * +        * 1    DMA Completion Interrupt for Channel 1 NOT USED
   * +        * 2    DMA Completion Interrupt for Channel 2 NOT USED
   * +        * 3    DMA Completion Interrupt for Channel 3 NOT USED
   * +        * 4    DMA Completion Interrupt for Channel 4
   * +        * 5    DMA Completion Interrupt for Channel 5
   * +        * 6    DMA Completion Interrupt for Channel 6
   * +        * 7    DMA Completion Interrupt for Channel 7
   * +        * 8    Display Rendering Related Interrupt A (RRA) NOT USED
   * +        * 9    Display Rendering Related Interrupt B (RRB) NOT USED
   * +        * 10   Display Non-Rendering Related Interrupt (NRR) NOT USED
   * +        * 11   Unused NOT USED
   * +        * 12   PMU Interrupt
   * +        * 13   Thermal Interrupt
   * +        * 14   SPI Interrupt NOT USED
   * +        * 15   Sbox Global APIC Error Interrupt
   * +        * 16   Machine Check Architecture (MCA) Interrupt
   * +        * 17   Remote DMA Completion Interrupt for Channel 0
   * +        * 18   Remote DMA Completion Interrupt for Channel 1
   * +        * 19   Remote DMA Completion Interrupt for Channel 2
   * +        * 20   Remote DMA Completion Interrupt for Channel 3
   * +        * 21   Remote DMA Completion Interrupt for Channel 4
   * +        * 22   Remote DMA Completion Interrupt for Channel 5
   * +        * 23   Remote DMA Completion Interrupt for Channel 6
   * +        * 24   Remote DMA Completion Interrupt for Channel 7
   * +        * 25   PSMI Interrupt NOT USED
   * +        */

  void IOApic::init() {
    ASSERT(base_address != nullptr);
    IOApic::IOAPIC_VERSION ver(read(IOApic::IOAPICVER));
    MLOG_DETAIL(mlog::boot, "IOAPIC init", DVAR(ver.version), DVAR(ver.max_redirection_table));

    RED_TABLE_ENTRY rte_irq;
    rte_irq.trigger_mode = 0;
    rte_irq.intpol = 0;

    // TODO: Spread across bigger range of interrupt vectors, as the priority of an interrupt is calculated from its vector
    for (size_t i = 0; i < ver.max_redirection_table + 1; i++) {
      rte_irq.intvec = BASE_IRQ + i;
      rte_irq.dest = 0; //to apic 0
      rte_irq.delmode = 0; // interrupt priority
      rte_irq.destmode = 0; // physical 0 / logical 1
      writeTableEntry(i, rte_irq);
    }
    sbox::enable_interrupts();
  }

  uint32_t IOApic::read(size_t reg) {
    return base_address[reg];
  }

  void IOApic::write(size_t reg, uint32_t value) {
    base_address[reg] = value;
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
    ASSERT(irq > 31 && irq < 256);
    ASSERT(base_address != nullptr);
    auto table_entry = irq - IOApic::BASE_IRQ;
    IOApic::RED_TABLE_ENTRY rte = readTableEntry(table_entry);
    rte.int_mask = 1;
    writeTableEntry(table_entry, rte);
  }

  /// Same as maskIRQ
  void IOApic::unmaskIRQ(uint64_t irq) {
    ASSERT(irq > 31 && irq < 256);
    ASSERT(base_address != nullptr);
    auto table_entry = irq - IOApic::BASE_IRQ;
    IOApic::RED_TABLE_ENTRY rte = readTableEntry(table_entry);
    rte.int_mask = 0;
    writeTableEntry(table_entry, rte);
  }

} // namespace mythos
