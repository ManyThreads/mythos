#include "cpu/IOApic.hh"

namespace mythos {


  void IOApic::init() {
    IOAPIC_VERSION ver(read(IOApic::IOAPICVER));
    MLOG_ERROR(mlog::boot, "Read ver value", DVAR(ver.version), DVAR(ver.max_redirection_table));
    MLOG_ERROR(mlog::boot, DVAR(read(IOApic::IOAPICVER)));

    RED_TABLE_ENTRY rte_irq;
    rte_irq.trigger_mode = 0;
    rte_irq.intpol = 0;

    RED_TABLE_ENTRY rte_pci; // interrupts 16..19
    rte_pci.trigger_mode = 1; // level triggered
    rte_pci.intpol = 1; // low active

    for (size_t i = 0; i < ver.max_redirection_table + 1; i++) {
      rte_irq.dest = 0x21 + i;
      write(IOApic::IOREDTBL_BASE+2*i, (uint32_t) rte_irq.lower);
      write(IOApic::IOREDTBL_BASE+2*i+1, (uint32_t) rte_irq.upper);
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


} // namespace mythos

