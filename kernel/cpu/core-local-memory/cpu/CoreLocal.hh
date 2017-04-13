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

#include "util/compiler.hh"
#include <cstdint>
#include <cstddef>

#define KERNEL_CLM      __attribute__((section (".kernel_clm")))
#define KERNEL_CLM_HOT  __attribute__((section (".kernel_clm.hot")))

namespace mythos {

  class KernelCLM
  {
  public:
    static void init(size_t size) {
      blockSize = size;
      offset = 0;
    }
    static size_t getBlockSize() { return blockSize; }
    static size_t getOffset(size_t threadID) { return threadID*blockSize; }
    static size_t getOffset() { return get(&offset); }
    static void initOffset(size_t threadID) { set(&offset, threadID*blockSize); }

    template<class T>
    ALWAYS_INLINE static void set(uintptr_t& var, T* value) {
      setWord(reinterpret_cast<size_t*>(&var), reinterpret_cast<size_t>(value));
    }

    ALWAYS_INLINE static void set(size_t* var, size_t value) {
      setWord(var, value);
    }

    ALWAYS_INLINE static void setAt(size_t threadID, size_t* var, size_t value) {
      char *ptr = reinterpret_cast<char*>(var) + threadID*blockSize;
      *reinterpret_cast<size_t*>(ptr) = value;
    }

    template<class T>
    ALWAYS_INLINE static T* get(uintptr_t const& var) {
      return reinterpret_cast<T*>(getWord(reinterpret_cast<size_t const*>(&var)));
    }

    ALWAYS_INLINE static size_t get(size_t const* var) {
      return getWord(var);
    }

    ALWAYS_INLINE static size_t getAt(size_t threadID, size_t const* var) {
      char const *ptr = reinterpret_cast<char const*>(var) + threadID*blockSize;
      return *reinterpret_cast<size_t const*>(ptr);
    }

    template<typename T>
    ALWAYS_INLINE static T* toLocal(T* var) {
      return reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(var)+getOffset());
    }

  protected:
    static void setWord(uint64_t* addr, uint64_t value) {
      asm("movq %1,%%gs:%0" : : "m" (*addr), "ri" (value));
    }

    static uint64_t getWord(uint64_t const* addr) {
      uint64_t value;
      asm("movq %%gs:%1,%0" : "=r" (value) : "m" (*addr));
      return value;
    }

    static size_t blockSize;
    static size_t offset KERNEL_CLM_HOT;
  };

  template<typename T>
  class CoreLocal
  {
  public:
    ALWAYS_INLINE T const* addr() const { return KernelCLM::toLocal(&var); }
    ALWAYS_INLINE T* addr() { return KernelCLM::toLocal(&var); }
    ALWAYS_INLINE T const* operator->() const { return addr(); }
    ALWAYS_INLINE T* operator->() { return addr(); }
    ALWAYS_INLINE T& operator*() { return *addr(); }
    ALWAYS_INLINE T const& operator*() const { return *addr(); }

    ALWAYS_INLINE T const& get() const { return *addr(); }
    ALWAYS_INLINE void set(T const& value) { *addr() = value; }
  protected:
    T var;
  };

  template<>
  class CoreLocal<size_t>
  {
  public:
    ALWAYS_INLINE size_t get() const { return KernelCLM::get(&var); }
    ALWAYS_INLINE void set(size_t value) { return KernelCLM::set(&var, value); }
    // ALWAYS_INLINE size_t operator[](size_t threadID) { return KernelCLM::getAt(threadID, &var); }
    // ALWAYS_INLINE void setAt(size_t threadID, size_t value) { KernelCLM::setAt(threadID, &var, value); }
    ALWAYS_INLINE CoreLocal& operator= (size_t value) { set(value); return *this; }
    ALWAYS_INLINE operator size_t () const { return get(); }
  protected:
    size_t var;
  };

  template<typename T>
  class CoreLocal<T*>
  {
  public:
    ALWAYS_INLINE T* get() const { return KernelCLM::get<T>(var); }
    ALWAYS_INLINE void set(T* value) { KernelCLM::set<T>(var, value); }
    // ALWAYS_INLINE T* operator[](size_t threadID) {
    //   return reinterpret_cast<T*>(KernelCLM::getAt(threadID, reinterpret_cast<size_t const*>(&var)));
    // }
    // ALWAYS_INLINE void setAt(size_t threadID, T* value) {
    //   KernelCLM::setAt(threadID, reinterpret_cast<size_t*>(&var), reinterpret_cast<size_t>(value));
    // }
    ALWAYS_INLINE T* operator-> () const { return get(); }
    ALWAYS_INLINE T& operator* () const { return *get(); }
    ALWAYS_INLINE CoreLocal& operator= (T* value) { set(value); return *this; }
    // ALWAYS_INLINE operator T* () const { return get(); }
  protected:
    uintptr_t var; 
  };

  template<>
  class CoreLocal<void*>
  {
  public:
    ALWAYS_INLINE void* get() const { return KernelCLM::get<void>(var); }
    ALWAYS_INLINE void set(void* value) { KernelCLM::set<void>(var, value); }
    ALWAYS_INLINE CoreLocal& operator= (void* value) { set(value); return *this; }
    ALWAYS_INLINE operator void* () const { return get(); }
  protected:
    uintptr_t var; 
  };
  
} // namespace mythos
