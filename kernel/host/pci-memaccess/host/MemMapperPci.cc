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
/* -*- mode:C++; -*- */

#include "host/MemMapperPci.hh"
#include "util/alignments.hh"

#include <string>
#include <iostream>
#include <sys/mman.h> // for mmap
#include <cstdint> // for uint64_t
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstdlib>
#include <sys/stat.h>
#include <unistd.h> // for close()
#include <stdexcept>

namespace mythos {

  MemMapperPci::MemMapperPci(std::string resource)
  {
    aper_fd = ::open(resource.c_str(), O_RDWR);
    if (aper_fd == -1) throw std::runtime_error("could not open memory");
  }
  
  MemMapperPci::~MemMapperPci()
  {
    ::close(aper_fd);
  }
    
  void* MemMapperPci::map(PhysPtr<void> paddr, size_t len)
  {
    size_t base = Align4k::round_down(paddr.physint());
    size_t pagelen = Align4k::round_up(len+paddr.physint()-base);
    void* aper_va = ::mmap(0, pagelen, PROT_READ|PROT_WRITE, MAP_SHARED, aper_fd, base);
    std::cout << "mmap " << (void*)paddr.physint() << " size " << len
	      << " base " << (void*)base << " pagelen " << pagelen
	      << " aper_va " << aper_va << std::endl;
    if (aper_va == MAP_FAILED) throw std::runtime_error("mmap failed");
    return ((char*)aper_va)+paddr.physint()-base;
  }

  void MemMapperPci::unmap(void const* vaddr, size_t len)
  {
    size_t base = Align4k::round_down(size_t(vaddr));
    size_t pagelen = Align4k::round_up(len+size_t(vaddr)-base);
    ::munmap((void*)base, pagelen);
  }

} // namespace mythos
