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

#include "util/PhysPtr.hh"
#include <cstdint> // for uint64_t
#include <utility> // for std::move

namespace mythos {

  namespace SFI {

    enum Constants {
      SFI_PHYS = 0x00000000000E0000,
      SFI_PHYS_END = 0x0000000000100000,
      SFI_SIZE = SFI_PHYS_END - SFI_PHYS
    };

    struct Base {
      char signature[4];
      uint32_t length;
      uint8_t revision;
      uint8_t checksum;
      char oemID[6];
      uint64_t oemTableID;

      bool isCorrectChecksum() const {
        uint8_t sum = 0;
        uint8_t const* data = reinterpret_cast<uint8_t const*>(this);
        for (unsigned i=0; i<length; i++) sum = uint8_t(sum + data[i]);
        return sum == 0;
      }
    };

    struct Cpus
      : public Base
    {
      uint32_t apicID[1];

      bool isCorrectType() const {
        return
          signature[0] == 'C' &&
          signature[1] == 'P' &&
          signature[2] == 'U' &&
          signature[3] == 'S';
      }

      unsigned size() const {
        return unsigned((length-sizeof(Base))/sizeof(uint32_t));
      }

      uint32_t operator[] (size_t i) const { return apicID[i]; }
    };

    struct Apic
      : public Base
    {
      uint64_t apicID[1];

      bool isCorrectType() const {
        return
          signature[0] == 'A' &&
          signature[1] == 'P' &&
          signature[2] == 'I' &&
          signature[3] == 'C';
      }

      unsigned size() const
      {
        return unsigned((length-sizeof(Base))/sizeof(uint64_t));
      }

      uint64_t operator[] (unsigned i) const { return apicID[i]; }
    };

    struct Mmap
      : public Base
    {
      enum EfiMemoryType {
        ReservedMemoryType,
        LoaderCode,
        LoaderData,
        BootServicesCode,
        BootServicesData,
        RuntimeServicesCode,
        RuntimeServicesData,
        ConventionalMemory,
        UnusableMemory,
        ACPIReclaimMemory,
        ACPIMemoryNVS,
        MemoryMappedIO,
        MemoryMappedIOPortSpace,
        PalCode,
        MaxMemoryType
      };

      // see http://wiki.phoenix.com/wiki/index.php/EFI_MEMORY_DESCRIPTOR
      // see http://wiki.phoenix.com/wiki/index.php/EFI_MEMORY_TYPE
      struct Descriptor {
        PhysPtr<void> getAddress() const { return PhysPtr<void>(phys_addr); }
        uint64_t getSize() const { return num_pages*4096; }

        uint32_t type;
        uint64_t phys_addr;
        uint64_t virt_addr;
        uint64_t num_pages;
        uint64_t attribute;
      } __attribute__((packed));

      Descriptor map[1];

      bool isCorrectType() const {
        return
          signature[0] == 'M' &&
          signature[1] == 'M' &&
          signature[2] == 'A' &&
          signature[3] == 'P';
      }

      unsigned size() const {
        return unsigned((length-sizeof(Base))/sizeof(Descriptor));
      }

      Descriptor operator[] (unsigned i) const { return map[i]; }
    };

    struct Syst
      : public Base
    {
      PhysPtr<Base> entry[1];

      bool isCorrectType() const {
        return
          signature[0] == 'S' &&
          signature[1] == 'Y' &&
          signature[2] == 'S' &&
          signature[3] == 'T';
      }

      unsigned size() const {
        return unsigned((length-sizeof(Base))/sizeof(uint64_t));
      }

      Base* operator[] (unsigned i) const { return entry[i].log(); }

      static Syst* find(uint64_t start, uint64_t length)
      {
        for (PhysPtr<Syst> addr(start); addr < PhysPtr<Syst>(start+length); addr.incbytes(16))
          if (addr->isCorrectType()) return addr.log();
        return 0;
      }

      static Syst* find() { return find(SFI_PHYS, SFI_SIZE); }

      Mmap* getMmap()
      {
        for(unsigned i=0; i<size(); ++i) {
          Mmap* mmap = static_cast<Mmap*>(entry[i].log());
          if(mmap->isCorrectType()) return mmap;
        }
        return nullptr;
      }

      Cpus* getCpus()
      {
        for(unsigned i=0; i<size(); ++i) {
          Cpus* cpus = static_cast<Cpus*>(entry[i].log());
          if(cpus->isCorrectType()) return cpus;
        }
        return nullptr;
      }
    };

  } // namespace SFI

  class SFIInfo
  {
    public:
      typedef SFI::Syst const Syst;

      SFIInfo()
      {
        syst = SFI::Syst::find();
        if (syst) {
          mmap = syst->getMmap();
          cpus = syst->getCpus();
        }
      }

      virtual ~SFIInfo() {}

      bool hasSyst() const { return syst != nullptr; }
      bool hasMmap() const { return mmap != nullptr; }
      bool hasCpus() const { return cpus != nullptr; }

      SFI::Mmap* getMmap()
      {
        ASSERT_MSG(mmap, "Missing SFI memory map information.");
        return mmap;
      }

      size_t numThreads() const
      {
        ASSERT_MSG(cpus, "Missing SFI cpu information.");
        return cpus->size();
      }
      size_t threadID(size_t idx) const
      {
        ASSERT_MSG(cpus, "Missing SFI cpu information.");
        return (*cpus)[idx];
      }

    protected:
      SFI::Syst* syst;
      SFI::Mmap* mmap;
      SFI::Cpus* cpus;
  };

} // namespace mythos
