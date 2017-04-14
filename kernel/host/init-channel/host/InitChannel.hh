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
#pragma once

#include "mythos/HostInfoTable.hh"
#include <cstddef>
#include <cstdint>

namespace mythos {

  /** establishes communication to the mythos init appliction via shared memory */
  class InitChannel
  {
  public:
    InitChannel() {}
    /** mmap the relevant pages. the file descriptor should point to the
     * device memory file of the XeonPhi.
     */
    InitChannel(int fd) { open(fd); }
    InitChannel(char const* fname)  { open(fname); }
    ~InitChannel() { close(); }

    void open(char const* fname);

    void open(int fd) {
      if (this->fd != 0) close();
      this->fd = fd;
      closefd = false;
    }

	int getFD(){ return fd; }

    void close();

    struct InfoPtr {
      uint64_t info;
      uint64_t debug;
    };

    /** maps the host info pointer structure at physical address 2MiB of the KNC */
    InfoPtr* getHITptr();

    HostInfoTable* getHIT();

    HostInfoTable::DebugChannel* getDebugOut();

    void* getInitMem();

    /** get the memory size, only valid after successful \m getInitMem(). */
    size_t getInitMemSize() const { return initMemSize; }

  public:
    void* map(uint64_t paddr, size_t size);
    void unmap(void* vaddr, size_t size);

    template<typename T>
    T round_up(T v, T a=4096) const { return ((v+a-1)/a)*a; }
    template<typename T>
    T round_down(T v, T a=4096) const { return (v/a)*a; }

    static void clflush(void* addr) { asm volatile("clflush (%0)"::"r"(addr)); }

  protected:
    int fd = 0;
    bool closefd = false;
    InfoPtr* hostInfoTablePtr = nullptr;
    HostInfoTable* hostInfoTable = nullptr;
    HostInfoTable::DebugChannel* debugOutChannel = nullptr;
    void* initMem = nullptr;
    size_t initMemSize = 0;
  };

} // namespace mythos
