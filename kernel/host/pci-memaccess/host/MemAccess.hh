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

#include "host/IMemMapper.hh"
#include <iostream>

namespace mythos {

  template<typename T>
  class MemAccess
  {
  public:
    MemAccess() : mapper(0), laddr(0), msize(0) {}
    MemAccess(IMemMapper* mapper, PhysPtr<T> paddr, size_t msize=sizeof(T))
      : mapper(mapper)
      , laddr(reinterpret_cast<T*>(this->mapper->map(paddr.asVoid(), msize)))
      , msize(msize)
    {
      std::cout << "map " << (void*)paddr.physint() << " size " << msize 
		<< " laddr " << laddr << std::endl;
      // mlog::mmu.detail("temp. map", DVARhex(paddr), DVARhex(laddr), DVAR(msize));
    }

    MemAccess(MemAccess&& o)
      : mapper(o.mapper), laddr(o.laddr), msize(o.msize)
    { o.mapper = 0; }

    template<typename Compatible>
    explicit MemAccess(MemAccess<Compatible>&& o)
      : mapper(o.mapper), laddr(o.laddr), msize(o.msize)
    { o.mapper = 0; }

    template<typename Compatible>
    explicit MemAccess(MemAccess<Compatible>& o)
      : mapper(o.mapper), laddr(o.laddr), msize(o.msize)
    { o.mapper = 0; }
    
    ~MemAccess() {
      if (mapper != 0) {
	mapper->unmap(laddr, msize);
	// mlog::mmu.detail("temp. unmap", DVARhex(laddr), DVAR(msize));
      }
    }

    MemAccess& operator= (MemAccess&& o) {
      if (mapper != 0) mapper->unmap(laddr, msize);
      mapper = o.mapper;
      laddr = o.laddr;
      msize = o.msize;
      o.mapper = 0;
      return *this;
    }

    T* addr() const { return laddr; }
    size_t size() const { return msize; }
    T* operator-> () const { return laddr; }
    T& operator* () const { return *laddr; }

  protected:
    IMemMapper* mapper;
    T* laddr;
    size_t msize;
  };
  
} // namespace mythos
