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

#include "boot/memory-layout.h"
#include "util/compiler.hh"
#include "util/assert.hh"
#include <cstddef> // for size_t
#include <cstdint>

#include "boot/ihk-entry.hh"

//#include "objects/mlog.hh"

namespace mythos {

  extern char KERN_END SYMBOL("KERN_END");

  inline bool isKernelAddress(const void* ptr)
  {
    auto p = reinterpret_cast<uintptr_t>(ptr);
    return (p >= KERNELMEM_ADDR) && (p < KERNELMEM_ADDR + KERNELMEM_SIZE);
  }

  inline bool isImageAddress(const void* ptr)
  {
    auto p = reinterpret_cast<uintptr_t>(ptr);
    return (p >= VIRT_ADDR) && (p < VIRT_ADDR + uintptr_t(&KERN_END));
  }

  template<typename T>
  class PACKED PhysPtr
  {
  public:
    PhysPtr() : ptr(0) {}

    PhysPtr(const PhysPtr<T>&) = default;

    explicit PhysPtr(uint64_t ptr) : ptr(ptr) {}

    static PhysPtr fromImage(T* vp) {
      ASSERT(isImageAddress(vp));
      return PhysPtr(reinterpret_cast<uintptr_t>(vp) - MAP_KERNEL_START + mythos::boot::x86_kernel_phys_base);
    }

    static PhysPtr fromKernel(T* vp) {
      ASSERT(isKernelAddress(vp));
      return PhysPtr(reinterpret_cast<uintptr_t>(vp) - KERNELMEM_ADDR + mythos::boot::x86_kernel_phys_base);
    }

    static PhysPtr fromPhys(T* ptr) {
      return PhysPtr(reinterpret_cast<uintptr_t>(ptr));
    }

    explicit operator bool() const { return ptr != 0; }
    bool operator==(PhysPtr rhs) const { return ptr == rhs.ptr; }
    bool operator!=(PhysPtr rhs) const { return ptr != rhs.ptr; }
    bool operator<(PhysPtr rhs) const { return ptr < rhs.ptr; }
    bool operator<=(PhysPtr rhs) const { return ptr <= rhs.ptr; }
    bool operator>=(PhysPtr rhs) const { return ptr >= rhs.ptr; }
    bool operator>(PhysPtr rhs) const { return ptr > rhs.ptr; }

    PhysPtr& operator+=(size_t inc) { ptr += sizeof(T)*inc; return *this; }
    PhysPtr operator+(size_t inc) const { return PhysPtr(ptr+sizeof(T)*inc); }
    PhysPtr& incbytes(size_t inc) { ptr += inc; return *this; }
    PhysPtr plusbytes(size_t inc) const { return PhysPtr(ptr+inc); }

    PhysPtr& operator-=(size_t inc) { ptr -= sizeof(T)*inc; return *this; }
    PhysPtr operator-(size_t inc) const { return PhysPtr(ptr-sizeof(T)*inc); }
    size_t operator-(PhysPtr rhs) const { return (ptr-rhs)/sizeof(T); }

    PhysPtr& operator=(PhysPtr rhs) { ptr = rhs.ptr; return *this; }

    uintptr_t physint() const { return ptr; }

    uintptr_t logint() const {
      ASSERT(kernelmem());
      return ptr - mythos::boot::x86_kernel_phys_base + KERNELMEM_ADDR;
    }

    bool kernelmem() const { 
		//MLOG_INFO(mlog::km, "kernelmem", DVARhex(ptr), DVARhex(mythos::boot::x86_kernel_phys_base));
		return (ptr - mythos::boot::x86_kernel_phys_base) < KERNELMEM_SIZE; }

    bool canonical() const
    {
      static constexpr uintptr_t CANONICAL_MASK = ((1ull << (64 - 48))-1) << 48;
      return (ptr & CANONICAL_MASK) == 0 || (ptr & CANONICAL_MASK) == CANONICAL_MASK;
    }

    T* log() const { return reinterpret_cast<T*>(logint()); }
    T* operator->() const { return log(); }

  protected:
    uintptr_t ptr;
  };

  template<class T>
  PhysPtr<T> physPtr(T* ptr) { return PhysPtr<T>::fromPhys(ptr); }

  template<class T>
  PhysPtr<T> physPtrFromImage(T* ptr) { return PhysPtr<T>::fromImage(ptr); }

  template<class T>
  PhysPtr<T> physPtrFromKernel(T* ptr) { return PhysPtr<T>::fromKernel(ptr); }


  template<class T>
  T& operator*(PhysPtr<T> ptr) { return *ptr.log(); }

  template<typename T>
  class PACKED PhysPtr32
  {
  public:
    PhysPtr32() : ptr(0) {}

    PhysPtr32(const PhysPtr32<T>&) = default;
    
    explicit PhysPtr32(uint32_t ptr) : ptr(ptr) {}
    explicit PhysPtr32(uint64_t ptr) : ptr(uint32_t(ptr)) {
      ASSERT(ptr <= 0xFFFFFFFF);
    }

    explicit operator bool() const { return ptr != 0; }
    bool operator==(PhysPtr32 rhs) const { return ptr == rhs.ptr; }
    bool operator!=(PhysPtr32 rhs) const { return ptr != rhs.ptr; }
    bool operator<(PhysPtr32 rhs) const { return ptr < rhs.ptr; }
    bool operator<=(PhysPtr32 rhs) const { return ptr <= rhs.ptr; }
    bool operator>=(PhysPtr32 rhs) const { return ptr >= rhs.ptr; }
    bool operator>(PhysPtr32 rhs) const { return ptr > rhs.ptr; }

    PhysPtr32& operator+=(size_t inc) { ptr += sizeof(T)*inc; return *this; }
    PhysPtr32 operator+(size_t inc) const { return PhysPtr32(ptr+sizeof(T)*inc); }
    PhysPtr32& incbytes(size_t inc) { ptr += inc; return *this; }
    PhysPtr32 plusbytes(size_t inc) const { return PhysPtr32(ptr+inc); }

    PhysPtr32& operator-=(size_t inc) { ptr -= sizeof(T)*inc; return *this; }
    PhysPtr32 operator-(size_t inc) const { return PhysPtr32(ptr-sizeof(T)*inc); }
    size_t operator-(PhysPtr32 rhs) const { return (ptr-rhs)/sizeof(T); }

    PhysPtr32& operator=(PhysPtr32 rhs) { ptr = rhs.ptr; return *this; }

    void* phys() const { return reinterpret_cast<void*>(ptr); }
    uintptr_t physint() const { return ptr; }

    uintptr_t logint() const { return uintptr_t(ptr) + KERNELMEM_ADDR; }
    T* log() const { return reinterpret_cast<T*>(logint()); }
    T* operator->() const { return log(); }

  protected:
    uint32_t ptr;
  };

  template<class T>
  T& operator*(PhysPtr32<T> ptr) { return *ptr.log(); }

  template<class T>
  uint32_t kernel2phys(T* vp) {
    ASSERT(isKernelAddress(vp));
    return uint32_t(reinterpret_cast<uintptr_t>(vp) - KERNELMEM_ADDR + mythos::boot::x86_kernel_phys_base);
  }

  template<class T>
  uint32_t kernel2offset(T* vp) {
    ASSERT(isKernelAddress(vp));
    return uint32_t(reinterpret_cast<uintptr_t>(vp) - KERNELMEM_ADDR);
  }

  template<class T>
  T* offset2kernel(uint32_t o) {
    ASSERT(o < KERNELMEM_SIZE);
    return reinterpret_cast<T*>(o + KERNELMEM_ADDR);
  }

  template<class T>
  T* phys2kernel(uintptr_t phys) {
    return reinterpret_cast<T*>(phys - mythos::boot::x86_kernel_phys_base + KERNELMEM_ADDR);
  }

  template<class T>
  T* image2kernel(T* vp) {
    ASSERT(isImageAddress(vp));
    return reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(vp) - VIRT_ADDR + KERNELMEM_ADDR);
  }

} // namespace mythos
