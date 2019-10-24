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

#include "util/PhysPtr.hh"

namespace mythos {

  /**
   * Helper class for passing alignment requirements as arguments
   */
  class AlignmentObject
  {
  public:
    constexpr AlignmentObject(size_t Align)
      : Align(Align) {}

    constexpr size_t alignment() const { return Align; }
    constexpr bool is_aligned(size_t addr) const { return (addr % Align == 0); }
    inline constexpr size_t round_up(size_t addr) const { return ((addr+Align-1)/Align)*Align; }
    inline constexpr size_t round_down(size_t addr) const { return (addr/Align)*Align; }
    inline constexpr size_t next(size_t addr) const { return round_down(addr+Align); }
    inline constexpr size_t prev(size_t addr) const { return round_up(addr-Align); }

    template<typename T>
    constexpr bool is_aligned(T* addr) const { return is_aligned(size_t(addr)); }

    template<typename T>
    constexpr T* round_up(T* addr) const { return reinterpret_cast<T*>(round_up(size_t(addr))); }

    template<typename T>
    constexpr T* round_down(T* addr) const { return reinterpret_cast<T*>(round_down(size_t(addr))); }

    template<class T>
    inline bool is_aligned(PhysPtr<T> addr) const { return is_aligned(addr.physint()); }
    template<class T>
    inline PhysPtr<T> round_up(PhysPtr<T> addr) const { return PhysPtr<T>(round_up(addr.physint())); }
    template<class T>
    inline PhysPtr<T> round_down(PhysPtr<T> addr) const { return PhysPtr<T>(round_down(addr.physint())); }
    template<class T>
    inline PhysPtr<T> next(PhysPtr<T> addr) const { return round_down(addr+Align); }
    template<class T>
    inline PhysPtr<T> prev(PhysPtr<T> addr) const { return round_up(addr-Align); }

  protected:
    size_t Align;
  };


  template<size_t Align>
  class Alignment {
  public:
    // use this, if an object is required
    static constexpr AlignmentObject obj() { return AlignmentObject(Align); }

    static constexpr size_t alignment() { return Align; }
    static constexpr bool is_aligned(size_t addr) { return (addr % Align == 0); }
    static constexpr size_t round_up(size_t addr) { return ((addr+Align-1)/Align)*Align; }
    static constexpr size_t round_down(size_t addr) { return (addr/Align)*Align; }
    static constexpr size_t next(size_t addr) { return round_up(addr+1); }
    static constexpr size_t prev(size_t addr) { return round_down(addr-1); }

    template<typename T>
    static constexpr bool is_aligned(T* addr) {
      return is_aligned(reinterpret_cast<size_t>(addr));
    }

    template<typename T>
    static constexpr T* round_up(T* addr) {
      return reinterpret_cast<T*>(round_up(reinterpret_cast<size_t>(addr)));
    }

    template<typename T>
    static constexpr T* round_down(T* addr) {
      return reinterpret_cast<T*>(round_down(reinterpret_cast<size_t>(addr)));
    }

    template<class T>
    static inline bool is_aligned(PhysPtr<T> addr) { return is_aligned(addr.physint()); }
    template<class T>
    static inline PhysPtr<T> round_up(PhysPtr<T> addr) { return PhysPtr<T>(round_up(addr.physint())); }
    template<class T>
    static inline PhysPtr<T> round_down(PhysPtr<T> addr) { return PhysPtr<T>(round_down(addr.physint())); }
    template<class T>
    static inline PhysPtr<T> next(PhysPtr<T> addr) { return round_down(addr+Align); }
    template<class T>
    static inline PhysPtr<T> prev(PhysPtr<T> addr) { return round_up(addr-Align); }
  };

  typedef Alignment<(1ull)> AlignByte;
  typedef Alignment<sizeof(size_t)> AlignWord;
  typedef Alignment<sizeof(size_t)*8> AlignLine;
  typedef Alignment<(1ull << 12)> Align4k;
  typedef Alignment<(1ull << 22)> Align4M;
  typedef Alignment<(1ull << 21)> Align2M;
  typedef Alignment<(1ull << 30)> Align1G;
  typedef Alignment<(1ull << 39)> Align512G;

} // namespace mythos
