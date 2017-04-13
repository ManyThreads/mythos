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

#include <cstddef> // for size_t
#include <cstdint>

namespace mythos {

  template<typename T>
  class PhysPtr
  {
  public:
    PhysPtr() : ptr(0) {}
    explicit PhysPtr(void* ptr) : ptr(reinterpret_cast<uintptr_t>(ptr)) {}
    // explicit PhysPtr(int32_t ptr) : ptr(ptr) {}
    // explicit PhysPtr(uint32_t ptr) : ptr(ptr) {}
    // explicit PhysPtr(int64_t ptr) : ptr(ptr) {}
    explicit PhysPtr(uint64_t ptr) : ptr(ptr) {}

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

    void* phys() const { return reinterpret_cast<void*>(ptr); }
    uintptr_t physint() const { return ptr; }

    PhysPtr<void> asVoid() const { return PhysPtr<void>(ptr); }
  protected:
    uintptr_t ptr;
  };

  template<class T>
  PhysPtr<T> physPtr(T* ptr) { return PhysPtr<T>(ptr); }

  // template<class T>
  // T& operator*(const PhysPtr<T>& ptr) { return *ptr.log(); }
  
} // namespace mythos 
