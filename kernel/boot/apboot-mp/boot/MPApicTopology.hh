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
 * Copyright 2016 Martin Messer, BTU Cottbus-Senftenberg
 */
#pragma once

#include "util/PhysPtr.hh"
#include "util/assert.hh"
#include "boot/mlog.hh"
#include <cstdint> // uint64_t

namespace mythos {

  // for information about intel MP specification tabel
  // look at http://www.intel.com/design/pentium/datashts/24201606.pdf
  namespace IntelMP{

    struct ConfigurationTable;

    struct FPStructure {
      char signature[4]; // "_MP_"
      PhysPtr32<ConfigurationTable> configuration_table;
      uint8_t length; // in length*16 bytes
      uint8_t specRev;
      uint8_t checksum; // should make all bytes zero, when added together -> see wiki.osdev.org/SMP
      uint8_t default_configuration;
      uint32_t features;
    };

    struct ConfigurationTable {
      char signature[4]; // "PCMP"
      uint16_t base_table_length;
      uint8_t specRev;
      uint8_t checksum; //look at mp_floating_pointer_structure
      char oem_id[8];
      char product_id[12];
      uint32_t oem_table;
      uint16_t oem_table_size;
      uint16_t entry_count; // count of entries after this table
      uint32_t lapic_address; // mmaddress of local APICs
      uint16_t extended_table_length;
      uint8_t extended_table_checksum;
      uint8_t reserved;
    };

    // extracted from entry_io_apic and entry_processor
    // because of different sizes of each processor and io entry.
    struct BaseEntry {
      uint8_t type;
    };

    struct ExtendedEntry {
      uint8_t type;   // types greater than 128 are extended entry types
      uint8_t length; // lenght of the extended entry
    };

    struct EntryProcessor : public BaseEntry {
      uint8_t local_apic_id;
      uint8_t local_apic_version;
      uint8_t flags; // bit 0 not set => ignore processor; bit 1 set => bootstrap processor
      uint32_t signature;
      uint32_t feature_flags;
      uint64_t reserved;
    }; // at least 20 byte

    struct EntryBus : public BaseEntry {
      uint8_t bus_id;
      char bus_type_string[6] ; // 48 bits
    };

    struct EntryIOApic : public BaseEntry {
      uint8_t id;
      uint8_t version;
      uint8_t flags;
      uint32_t address; // mmap adress of the io APIC
    };

    struct EntryIOInterrupt : public BaseEntry {
      uint8_t interrupt_type; // look at  enum interruptTypes for specific type
      // bits 0 and 1 for polarity of APIC IO input signals
      // bits 2 and 3 for trigger mode of APIC I/O signals (look at intel.com specification)
      uint16_t flags;
      uint8_t source_bus_id;
      uint8_t source_bus_irq;
      uint8_t destination_io_apic_id;
      uint8_t destination_io_apic_intin;
    };

    struct EntryLocalInterrupt : public BaseEntry {
      uint8_t interrupt_type; // interruptTypes
      uint16_t flags; // same as entry_io_interrupt flags ,but only for local APIC
      uint8_t source_bus_id;
      uint8_t source_bus_irq;
      uint8_t destination_local_apic_id;
      uint8_t destination_local_apic_intin;
    };

    struct EntrySystemAddressSpaceMapping : public ExtendedEntry {
      uint8_t bus_id;
      uint8_t address_type; // look at enum addressTypes for specific type
      uint64_t base_address;
      uint64_t address_lenght; // number of addresses which ar visible to the bus
    };

    struct EntryBusHierarchyDescriptor : public ExtendedEntry {
      uint8_t bus_id;
      uint8_t flags;
      uint8_t parent_bus_id;
    };

    struct EntryCompatibilityBusAddressSpaceModifier : public ExtendedEntry {
      uint8_t bus_id;
      uint8_t flags;
      uint32_t adress_range;
    };
  } // namespace IntelMP

  class MPApicTopology
  {
  public:
    MPApicTopology() {
      mlog::boot.info("searching IntelMP structure...");
      IntelMP::FPStructure* fp= findMP(PhysPtr<char>(0x0f0000ul).log(),0x0fffff-0x0f0000);
      ASSERT(fp);
      mlog::boot.detail("MP floating pointer base:", fp);
      mlog::boot.detail("MP floating spezification version:", fp->specRev);

      // find config table and local apics in base entries
      IntelMP::ConfigurationTable* configTable = fp->configuration_table.log();
      mlog::boot.detail("MP configuration spezification version:", configTable->specRev);
      mlog::boot.detail("MP configuration base length:", configTable->base_table_length);
      mlog::boot.detail("MP configuration entry count:", configTable->entry_count);

      uintptr_t baseEntries = uintptr_t(configTable) + sizeof(IntelMP::ConfigurationTable);
      uint16_t entry_count = 0;
      num_threads=0;
      for(uintptr_t pos=0; pos < configTable->base_table_length && entry_count < configTable->entry_count;){
        auto entry = reinterpret_cast<IntelMP::BaseEntry*>(baseEntries+pos);
        switch (entry->type){
          case PROCESSOR: {
            //mlog::boot.detail("PROCESSOR table entry at offset", pos);
            auto proc_entry = static_cast<IntelMP::EntryProcessor*>(entry);
            if (proc_entry->flags & 0x01) {  // processor is usuable
              lapicIDs[num_threads] = proc_entry->local_apic_id;
              num_threads++;
            }
            pos+=20;
            entry_count++;
            break;
          }
          default:
            //mlog::boot.detail("invalid table type", entry->type,"at offset", pos);
            pos+=8;
            entry_count++;
            break;
        }
      }
    }

    size_t numThreads() const { return num_threads; }
    size_t threadID(size_t idx) const { return lapicIDs[idx]; }

  protected:

    IntelMP::FPStructure* findMP(void* start, size_t len) {
      // we have to find structure table on 16 byte boudary
      unsigned int* search=(unsigned int*) start;
      unsigned int* end= search+len;
      for(;search<end;search+=4){
        if(search[0]==_MP_) return(IntelMP::FPStructure*) search;
      }
      return NULL;
    }

    enum EntryTypes {
      PROCESSOR=0,
      BUS,
      IOAPIC,
      IOINTASSIGN, // io interrupt assignment
      LOCALINTASSIGN, // local interrupt assignment
      // --------------------------------------- extendet mp configuration table
      SYSADDRMAPPING=128, // System Address Space Mapping
      BUSDESC=129, //bus hierarchy descriptor
      BUSADDRSPACEMOD=130 // compatibiliry bus address space modifier
    };

    enum InterruptTypes{
      INT=0,
      NMI,
      SMI,
      ExtlINT
    };

    enum AddressTypes{
      IO=0,
      MEMORY,
      PREFETCH
    };

    enum Signatures{
      _MP_=0x5F504D5F,// hex for "_MP_"
      PCMP=0x504D4350 // hex for "PCMP"
    };

    enum{ CPU_MAX=256};
    size_t num_threads;
    unsigned int lapicIDs[CPU_MAX];
  };
}
