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
 * Copyright 2016 Robert Kuban, Randolf Rotta, and contributors, BTU Cottbus-Senftenberg
 */
#pragma once

#include <cstdint>
#include <atomic>

namespace mythos {

  class CapEntry;

  class CapLink
  {
  public:
    CapLink() NOEXCEPT : ptr(0) {}
    CapLink(CapLink const& o) NOEXCEPT : ptr(o.ptr.load()) {}

    //bool isEmpty() const { return ptr.load() == 0; }

    void set(CapEntry* p, bool delmark) { this->ptr.store(pack(p,delmark)); }

    static uint32_t pack(CapEntry* p, bool delmark) {
      ASSERT_MSG((kernel2phys(p) & ~PTR_MASK) == 0, "kernel object without propper alignment");
      return kernel2phys(p) | (delmark?DELMARK:0);
    }

    CapEntry* deref() const { return phys2kernel<CapEntry>(ptr.load() & PTR_MASK); }
    CapEntry* operator-> () const { return deref(); }

    /** set the delete mark, even if the destination changes concurrently */
    void setMark() {
      ptr.fetch_or(DELMARK);
      // while (true) {
      // 	auto old = ptr.load();
      // 	if ((old&DELMARK) || ptr.compare_exchange_weak(old, old|DELMARK)) break;
      // }
    }

    bool isMarked() const { return ptr.load() | DELMARK; }

    bool equals(CapEntry* p, bool d) const { return ptr.load() == pack(p,d); }

    bool cas(CapEntry* oldPtr, CapEntry* newPtr) {
      ASSERT_MSG((kernel2phys(oldPtr) & ~PTR_MASK) == 0, "kernel object without propper alignment");
      ASSERT_MSG((kernel2phys(newPtr) & ~PTR_MASK) == 0, "kernel object without propper alignment");
      auto o = kernel2phys(oldPtr);
      return ptr.compare_exchange_strong(o, kernel2phys(newPtr));
    }

    bool cas(CapLink const& oldPtr, CapEntry* newPtr) {
      ASSERT_MSG((kernel2phys(newPtr) & ~PTR_MASK) == 0, "kernel object without propper alignment");
      auto o = oldPtr.ptr.load(std::memory_order_relaxed);
      return ptr.compare_exchange_strong(o, kernel2phys(newPtr));
    }

  private:
    std::atomic<uint32_t> ptr;

    static constexpr uint32_t DELMARK = 1;
    static constexpr uint32_t PTR_MASK = ~7;
  };

} // namespace mythos

