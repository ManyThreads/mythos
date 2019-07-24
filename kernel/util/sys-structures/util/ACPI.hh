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
 * Copyright 2014 Vlad, Randolf Rotta, Maik Krüger, and contributors, BTU Cottbus-Senftenberg 
 */
#pragma once

#include "util/compiler.hh"
#include "util/PhysPtr.hh"
#include <cstddef> // for size_t
#include <cstdint> // for uint32_t etc

namespace mythos {

  class PACKED ACPI
  {
  public:
    static bool sums_to_zero(void const* start, int len) {
      int sum = 0;
      char const* start_addr = (char const*) start;
      if (len == 0) return false;
      while (len--) sum += *start_addr++;
      return !(sum & 0xff);
    }

    enum {
      RSDT = 0x54445352, //CHARS_TO_UINT32('R','S','D','T'),
      XSDT = 0x54445358, //CHARS_TO_UINT32('X','S','D','T'),
      MADT = 0x43495041  //CHARS_TO_UINT32('A','P','I','C'),
    };

    uint32_t signature;
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    char oem_id[6];
    char oem_table_id[8];
    uint32_t oem_revision;
    uint32_t creator_id;
    uint32_t creator_revision;

    //bool check_type( uint32_t SDTtype );

    bool check_sum() const { return ACPI::sums_to_zero( this, length ); }
  };

  class PACKED APICEntry
  {
  public:
	// see spec table 5-19 for other values. what do we want to support?
    enum { LOCAL_APIC=0, IO_APIC=1, SOURCE_OVERRIDE=2 };
    uint8_t type; //our subclass tag
    uint8_t length;
    APICEntry* next() { return (APICEntry*)((char*)this + length); }
  };

  // spec 5.2.11.5, spec Table 5-21 and 5.2.1.1
  class PACKED LAPICEntry
    : public APICEntry
  {
  public:
    uint8_t apic_processor_id;
    uint8_t apic_id;
    uint32_t flags;
    bool isEnabled() const { return flags & 1; }
  };

  // spec 5.2.11.6
  class PACKED IOAPICEntry
    : public APICEntry
  {
  public:
    uint8_t   apic_id;
    uint8_t   reserved__;
    void*  apic_address;
    uint32_t global_system_interrupt_base; // Global system interrupt where INTI lines start
  };

  //spec 5.2.12.5
  class PACKED SourceOverrideEntry
    : public APICEntry
  {
  public:
    uint8_t bus;
    uint8_t source;
    uint32_t global_system_interrupt;
    uint16_t flags;
  };


  // Multiple APIC Description Table Structure, spec table 5-18 and 5.2.1.1
  class PACKED MADT
    : public ACPI
  {
  public:
    PhysPtr32<void> local_apic_address;
    uint32_t flags;

    APICEntry* begin() { return (APICEntry*)(this + 1);	}
    APICEntry* end() { return (APICEntry*)((char*)this + length); }
    bool has_PCAT_COMPAT() const { return flags & 1; }
  };

  // Root System Description Table, spec 5.2.7
  class PACKED RSDT
    : public ACPI 
  {
  public:
    PhysPtr32<ACPI> sdtPointers[1];

    /* return the number of contained pointers to other system description
     * tables (i.e. MADT) 
     * sizeof System_Description_Table should be 36 bytes (5.2.7).
     * The rest up to length is filled with n 4 byte addresses
     * pointing to other system description tables
     */
    size_t getNumSDTPtrs() { return (length - sizeof(ACPI)) / 4; }

    /* return the nth SDT pointer (starting with 0, it's C ;-)*/
    ACPI* getSDTPtr(size_t num) { return sdtPointers[num].log(); }
  };

  // Xtended System Description Table, spec 5.2.8
  class PACKED XSDT
    : public ACPI
  {
  public:
    PhysPtr<ACPI> sdtPointers[1];

    /* return the number of contained 64-bit pointers to other system
     * description tables (i.e. MADT) */
    size_t getNumSDTPtrs() { return ( length - sizeof( ACPI ) ) / 8; }

    /* return the nth SDT pointer (starting with 0, it's C ;-)*/
    ACPI* getSDTPtr(size_t num) { return sdtPointers[num].log(); }
  };

  // Root System Description Pointer Structure
  class PACKED RSDP
  {
  public:
    // ACPI RSDP version 1.0
    char signature[8];
    uint8_t checksum;
    char oemid[6];
    char revision;
    PhysPtr32<RSDT> rsdt_address;
    // ACPI RSDP version 2.0
    uint32_t length;
    PhysPtr<XSDT> xsdt_address; // spec v5.0: If the pointer to the XSDT is valid, the OS MUST use the XSDT instead of the RSDT.
    uint8_t extChecksum;
    uint8_t reserved[3];

    static RSDP* find(PhysPtr<char> start, size_t len) { return find(start.log(), len); }

    static RSDP* find(void* start, size_t len) {
      unsigned int* search = (unsigned int*) start;
      unsigned int* end = search + len/4;
      for (; search < end; search+=4 ) {
        if (search[0] != 0x20445352  ) continue; // "RSD "
        if (search[1] != 0x20525450 ) continue; // "PTR "
        if (((RSDP*)search)->check_sum()) return (RSDP*)search;
      }
      return 0;
    }

    // TODO seems to be wrong
    bool check_sum() const {
      return true;
      //return  ACPI::sums_to_zero(this, length);
    }

    bool is_ACPI_1_0() const { return revision == 0; }

    RSDT* getRSDTPtr() { return rsdt_address.log(); }
    XSDT* getXSDTPtr() { return xsdt_address.log(); }
  };

} // namespace mythos
