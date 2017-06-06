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

#include "util/compiler.hh"
#include "util/assert.hh"
#include <cstdint>
#include <cstddef>

namespace mythos {
  namespace elf64 {

    // based on information from https://www.uclibc.org/docs/elf-64-gen.pdf
    // and https://en.wikipedia.org/wiki/Executable_and_Linkable_Format
    struct PACKED Header
    {
      uint32_t magic; // 0x7F followed by ELF(45 4c 46)
      uint8_t sizeclass; // 1 or 2 to signify 32- or 64-bit format
      uint8_t endianness; // 1 or 2 to signify little or big endianness
      uint8_t elfversion; // 1 for the original version of ELF
      uint8_t osabi; // target operating system ABI: Linux=0x03
      uint8_t abiversion; // unused on linux
      uint8_t padding[7];
      uint16_t elftype; // 1,2,3,4: whether it's relocatable, executable, shared, or core
      uint16_t machine; // instruction set architecture: x86-64=0x3E, x86=0x03
      uint32_t version; // 1 for the original version of ELF
      uint64_t entry; // entry point from where the process starts
      uint64_t phoffset; // start of the program header table
      uint64_t shoffset; // start of the section header table
      uint32_t flags; // depends on the target architecture
      uint16_t ehsize; // size of this header, normally 64 bytes for 64-bit and 52 for 32-bit format
      uint16_t phentsize; // size of a program header table entry
      uint16_t phnum; // number of entries in the program header table
      uint16_t shentsize; // size of a section header table entry
      uint16_t shnum; // number of entries in the section header table
      uint16_t shstrndx; // index of the section header table entry that contains the section names

      bool isElf64Header() const {
        return magic == 0x464c457F && sizeclass == 2 && endianness == 1;
      }
    };

    enum Type { PT_NULL, PT_LOAD, PT_DYNAMIC, PT_INTERP, PT_NOTE, PT_SHLIB, PT_PHDR, PT_TLS };
    enum Flags { PF_X=1, PF_W=2, PF_R=4 };

    struct PACKED PHeader
    {
      uint32_t type;
      uint32_t flags;
      uint64_t offset; // offset inside the file
      uint64_t vaddr; // target virtual address in the memory
      uint64_t paddr; // reserved
      uint64_t filesize; // size inside the file
      uint64_t memsize; // target size in the memory
      uint64_t alignment;
    };

    class Elf64Image
    {
    public:
      Elf64Image(void* start) : start(uintptr_t(start)) {}
      bool isValid() const {
        ASSERT(header()->phentsize >= sizeof(PHeader));
        return header()->isElf64Header();
      }
      size_t phnum() const { return header()->phnum; }
      Header const* header() const { return reinterpret_cast<Header*>(start); }
      PHeader const* phdr(size_t idx) const {
        return reinterpret_cast<PHeader*>(start + header()->phoffset + idx*header()->phentsize);
      }

    protected:
      uintptr_t start;
    };
  } // namespace elf64
} // namespace mythos
