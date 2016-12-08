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
#pragma once

#include "util/PhysPtr.hh"
#include <cstdint> // for uint64_t

namespace mythos {
  
  namespace MultiBoot {
    enum Constants {
      MAGIC=0x2BADB002
    };

    struct AOutSymbols {
      uint32_t tabsize;
      uint32_t strsize;
      PhysPtr32<char> addr;
      uint32_t reserved;
    };

    struct ELFHeader {
      uint32_t num;
      uint32_t size;
      PhysPtr32<char> addr;
      uint32_t shndx;
    };

    struct MMapEntry {
      bool isAvailable() const { return type & 0x01; }
      bool isReserved() const {  return type & 0x02; }
      PhysPtr<void> getAddress() const { return PhysPtr<void>(addr); }
      uint64_t getSize() const { return len; }

      uint32_t size;
      uint64_t addr;
      uint64_t len;
      uint32_t type;
    } __attribute__((packed));

    struct Info {
      uint32_t getFlags() const { return flags; }
      bool hasMemorySize() const { return flags & (1 << 0); }
      bool hasBootDevice() const { return flags & (1 << 1); }
      bool hasCommandLine() const { return flags & (1 << 2); }
      bool hasModules() const { return flags & (1 << 3); }
      bool hasSymbols() const { return flags & (3 << 4); }
      bool hasMMap() const { return flags & (1 << 6); }
      
      uint64_t getLowMemorySize() const { return mem_lower*1024l; }
      uint64_t getHighMemorySize() const { return mem_upper*1024l; }

      uint32_t flags;
      uint32_t mem_lower;
      uint32_t mem_upper;
      uint32_t boot_device;
      uint32_t cmdline;
      uint32_t mods_count;
      uint32_t mods_addr;
      union {
	AOutSymbols aout_sym;
	ELFHeader elf_sec;
      } u;
      uint32_t mmap_length;
      PhysPtr32<MMapEntry> mmap_addr;
      uint32_t drives_length;
      uint32_t drives_addr;
      uint32_t config_table;
      uint32_t boot_loader_name;
      uint32_t apm_table;
      uint32_t vbe_control_info;
      uint32_t vbe_mode_info;
      uint16_t vbe_mode;
      uint16_t vbe_interface_seg;
      uint16_t vbe_interface_off;
      uint16_t vbe_interface_len;
    };

  } // namespace MultiBoot

  class MultiBootInfo
  {
  public:
    typedef MultiBoot::MMapEntry const MMapEntry;
    typedef MultiBoot::Info const Info;
    
    MultiBootInfo(uint64_t multiboot_table)
      : mbi(multiboot_table)
    {}

    uint32_t getMMapLength() const { return mbi->mmap_length; }

    MMapEntry* memmap_begin() const { return mbi->mmap_addr.log(); }
    MMapEntry* memmap_end() const { return mbi->mmap_addr.plusbytes(mbi->mmap_length).log(); }
    MMapEntry* memmap_next(MMapEntry* m) const {
      return reinterpret_cast<MMapEntry*>(uintptr_t(m) + m->size + 4);
    }

    template<class F>
    void foreach(F f)
    {
      for (MMapEntry* m = memmap_begin(); m < memmap_end(); m = memmap_next(m)) {
        f(m);
      }
    }

  protected:
    PhysPtr<Info> mbi;
  };

} // namespace mythos
