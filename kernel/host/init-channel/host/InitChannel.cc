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

#include "host/InitChannel.hh"
#include <sys/mman.h>  // for mmap()
#include <iostream>
#include <stdexcept>
#include <unistd.h>    // for close()
#include <sys/types.h> // for open()
#include <sys/stat.h>  // for open()
#include <fcntl.h>     // for open()

namespace mythos {

  void InitChannel::open(char const* fname)
  {
    if (fd != 0) close();
    fd = ::open(fname, O_RDWR);
    if (fd == -1) {
      fd = 0;
      throw std::runtime_error("could not open device memory file");
    }
    closefd = true;
  }

  void InitChannel::close()
  {
    unmap(hostInfoTablePtr, sizeof(*hostInfoTablePtr));
    unmap(hostInfoTable, sizeof(*hostInfoTable));
    unmap(debugOutChannel, sizeof(*debugOutChannel));
    unmap(initMem, initMemSize);
    hostInfoTablePtr = nullptr;
    hostInfoTable = nullptr;
    debugOutChannel = nullptr;
    initMem = nullptr;
    initMemSize = 0;
    if (closefd) ::close(fd);
    fd = 0;
  }

  void* InitChannel::map(uint64_t paddr, size_t size)
  {
    size_t base = round_down(paddr);
    size_t pagelen = round_up(size+paddr)-base;
    void* aper_va = ::mmap(0, pagelen, PROT_READ|PROT_WRITE, MAP_SHARED, fd, base);
    std::cerr << "mmap " << (void*)paddr << " size " << size
	      << " base " << (void*)base << " pagelen " << pagelen
	      << " aper_va " << aper_va << std::endl;
    if (aper_va == MAP_FAILED) throw std::runtime_error("mmap failed");
    return ((char*)aper_va)+paddr-base;
  }

  void InitChannel::unmap(void* vaddr, size_t size)
  {
    if (vaddr == 0) return;
    size_t base = round_down(size_t(vaddr));
    size_t pagelen = round_up(size+size_t(vaddr))-base;
    ::munmap((void*)base, pagelen);
  }

  InitChannel::InfoPtr* InitChannel::getHITptr()
  {
    if (hostInfoTablePtr == 0)
      hostInfoTablePtr = (InfoPtr*)map(0x200000ull, sizeof(*hostInfoTablePtr));
    return hostInfoTablePtr;
  }

  HostInfoTable* InitChannel::getHIT()
  {
    if (hostInfoTable == nullptr) {
      auto hitptr = getHITptr();
      if (!hitptr) return nullptr;
      clflush(&hitptr->info);
      if (!hitptr->info) return nullptr;
      hostInfoTable = (HostInfoTable*)map(hitptr->info, sizeof(*hostInfoTable));
    }
    return hostInfoTable;
  }

  HostInfoTable::DebugChannel* InitChannel::getDebugOut()
  {
    if (debugOutChannel == nullptr) {
      auto hit = getHIT();
      if (!hit) return nullptr;
      clflush(&hit->debugOut);
      if (!hit->debugOut) return nullptr;
      debugOutChannel = (HostInfoTable::DebugChannel*)map(hit->debugOut, sizeof(*debugOutChannel));
    }
    return debugOutChannel;
  }

  void* InitChannel::getInitMem()
  {
    if (initMem == nullptr) {
      auto hit = getHIT();
      if (!hit) return nullptr;
      clflush(&hit->initMem);
      if (!hit->initMem) return nullptr;
      initMemSize = hit->initMemSize;
      initMem = map(hit->initMem, initMemSize);
    }
    return initMem;
  }

} // namespace mythos
