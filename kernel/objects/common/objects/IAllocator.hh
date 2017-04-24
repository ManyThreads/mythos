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
 * Copyright 2016 Randolf Rotta, Robert Kuban, and contributors, BTU Cottbus-Senftenberg
 */
#pragma once

#include <cstddef>
#include <new>
#include "async/IResult.hh"
#include "util/error-trace.hh"

namespace mythos {

  struct MemoryDescriptor
  {
    MemoryDescriptor() : ptr(nullptr), size(0), alignment(0) {}
    MemoryDescriptor(size_t size, size_t alignment)
      : ptr(nullptr), size(size), alignment(alignment) {}
    MemoryDescriptor(void* ptr, size_t size)
      : ptr(ptr), size(size), alignment(0) {}
    template<class T>
    T* as() { return reinterpret_cast<T*>(ptr); }

    void* ptr;
    size_t size;
    size_t alignment;
  };

  class IAsyncFree
  {
  public:
    virtual ~IAsyncFree() {}

    /** called by child objects to return their memory chunks upon
     * their own final deletion */
    virtual void free(Tasklet* t, IResult<void>* r,
                      MemoryDescriptor* begin, MemoryDescriptor* end) = 0;

    /** called by usual child objects to return their memory upon
     * their own final deletion. */
    virtual void free(Tasklet* t, IResult<void>* r, void* start, size_t length) = 0;
  };

  class IAllocator
    : public IAsyncFree
  {
  public:
    virtual ~IAllocator() {}

    virtual optional<void*> alloc(size_t length, size_t alignment) = 0;
    virtual optional<void> alloc(MemoryDescriptor* begin, MemoryDescriptor* end) = 0;

    template<class T, size_t ALIGN=64, class... ARGS>
    optional<T*> create(ARGS const&... args) {
      auto ptr = alloc(sizeof(T), ALIGN);
      if (!ptr) RETHROW(ptr);
      return new(*ptr) T(this, args...);
    }

    using IAsyncFree::free;
    virtual void free(void* ptr, size_t length) = 0;
    virtual void free(MemoryDescriptor* begin, MemoryDescriptor* end) = 0;

    template<class T>
    void free(T* ptr) {
      ptr->~T(); // deconstruct
      free(ptr, sizeof(T)); /// @todo sizeof(T) is wrong when used on interface base classes!!
    }

    //virtual void release(IKernelObject* ptr) = 0;
  };

} // namespace mythos
