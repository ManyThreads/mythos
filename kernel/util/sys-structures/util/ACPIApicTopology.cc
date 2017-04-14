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
#include "util/ACPIApicTopology.hh"
#include "util/ACPI.hh"
#include "boot/mlog.hh"

namespace mythos {
  
  ACPIApicTopology::ACPIApicTopology()
    : systemDetected(false), n_threads(0)
  {
    MLOG_INFO(mlog::boot, "searching ACPI configuration...");

    // find root system description pointer
    // QEMU seabios places RSDP somewhere in low mem and it is ACPI 1.0
    PhysPtr<uint16_t> ebdaBase(0x040E);
    MLOG_DETAIL(mlog::boot, "EBDA base", (void*)(size_t(*ebdaBase)<<4));

    RSDP* rsdp = RSDP::find(PhysPtr<char>(0xe0000), 0x00100000-0xe0000);
    if (rsdp == 0) rsdp = RSDP::find(PhysPtr<char>(size_t(*ebdaBase)<<4), 4096);
    MLOG_DETAIL(mlog::boot, DVAR(rsdp));
    if (rsdp == 0 || rsdp->getRSDTPtr()==0) return;

    // parse the root system description table and search for the MADT
    RSDT* rsdt = rsdp->getRSDTPtr();
    MADT* madt = nullptr;
    for (size_t i = 0; i < rsdt->getNumSDTPtrs(); i++) {
      ACPI* entry = rsdt->getSDTPtr(i);
      MLOG_DETAIL(mlog::boot, "RSDT entry", DVARhex(entry->signature), DVAR(entry));
      if (entry->signature == ACPI::MADT) madt = static_cast<MADT*>(entry);
    }
    MLOG_DETAIL(mlog::boot, "parsing MADT entries ...", DVAR(madt));
    if (madt == nullptr) return;

    // parse the madt
    if (madt->has_PCAT_COMPAT()) disablePICs = true;

    for (APICEntry* entry=madt->begin(); entry<madt->end(); entry = entry->next()) {
      if (entry->length == 0) break;
      switch (entry->type) {
      case APICEntry::LOCAL_APIC: {
	LAPICEntry* lapic = static_cast<LAPICEntry*>(entry);
	if (!lapic->isEnabled()) break;
	if (n_threads == CPU_MAX) break;
	lapicIDs[n_threads] = lapic->apic_id;
	n_threads++;
	break;
      }
      case APICEntry::IO_APIC: {
	//IOAPICEntry* io_apic = (IOAPICEntry*) entry;
	// TODO should store the io-apic address!
	// ioapic_id = io_apic->apic_id;
	// unsigned int id = system_topology.ioapic_id << 4;
	// addr_t ioapic_regs_addr = Align4k::round_down(IO_APIC_REGISTERS_ADDRESS);
	// mmapper->mmap(ioapic_regs_addr, ioapic_regs_addr, Align4k::round_up(sizeof(int) * 2), IMemoryMapper::RWCD);
	// (*((volatile unsigned int*) 0xfec00000)) = 0;
	// (*((volatile unsigned int*) 0xfec00010)) = id;
	break;
      }
      case APICEntry::SOURCE_OVERRIDE: {
	// TODO this looks too strange!
	// SourceOverrideEntry* override = (SourceOverrideEntry*) entry;
	// if(override->bus != 0) break;
	// ioapic_irqs[override->global_system_interrupt] = override->source;
	// ioapic_irqs[override->source]--;
	break;
      }
      } // switch
    }
    MLOG_DETAIL(mlog::boot, "found apic IDs", DVAR(n_threads));
    systemDetected = true;
  }

} // namespace mythos
