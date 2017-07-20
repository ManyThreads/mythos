#include "cpu/IOApic.hh"
#include "cpu/sbox.hh"

namespace mythos {

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
    IOApic::IOAPIC_VERSION ver(read(IOApic::IOAPICVER));
    MLOG_INFO(mlog::boot, "IOAPIC init", DVAR(ver.version), DVAR(ver.max_redirection_table));


    RED_TABLE_ENTRY rte_irq;
    rte_irq.trigger_mode = 0;
    rte_irq.intpol = 0;

    for (size_t i = 0; i < ver.max_redirection_table + 1; i++) {
      rte_irq.intvec = 0x21 + i;
      rte_irq.dest = 0; //to apic 1
      rte_irq.delmode = 0; // interrupt priority
      rte_irq.destmode = 0; // physical 0 / logical 1
      write(IOApic::IOREDTBL_BASE+2*i+1, (uint32_t)rte_irq.upper);
      write(IOApic::IOREDTBL_BASE+2*i, (uint32_t)rte_irq.lower);
    }
    sbox::enable_interrupts();
  }

  uint32_t IOApic::read(size_t reg) {
    return base_address[reg];
  }

  void IOApic::write(size_t reg, uint32_t value) {
    base_address[reg] = value;
  }


} // namespace mythos
