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
 * Copyright 2016 Robert Kuban, Randolf Rotta, and contributors, BTU Cottbus-Senftenberg
 */
#pragma once

#include "mythos/caps.hh"
#include "util/PhysPtr.hh"
#include "util/compiler.hh"
#include "util/ostream.hh"
#include <cstdint>

namespace mythos {

  class IKernelObject;

  /** immutable capability value that contains a compressed pointer to
   * the capability's object (or sometimes subject), some inheritance
   * flags, and 32bit of rights meta-data for the object.
   */
  class Cap
  {
  public:
    enum CapFlags : uint8_t {
      NOFLAG = 0,
      DERIVED_FLAG = 1 << 0,
      REFERENCE_FLAG = 1 << 1,
      TRANSITION_FLAG = 1 << 2,
    };
    static constexpr uint8_t FLAG_MASK = 7;
    static constexpr uint8_t FLAG_BITS = 3;

    Cap() NOEXCEPT : _value(0) {}
    explicit Cap(CapValue _value) NOEXCEPT : _value(_value) {}

    explicit Cap(IKernelObject* ptr, CapData data = 0, CapFlags flags = NOFLAG) NOEXCEPT
    {
      f._ptr = _packPtr(ptr) & 0x1FFFFFFF;
      f.trans = (flags >> 2) & 1;
      f.ref = (flags >> 1) & 1;
      f.der = (flags >> 0) & 1;
      f._data = data;
    }

    IKernelObject* getPtr() const
    { return f._ptr ? phys2kernel<IKernelObject>(uint32_t(f._ptr << FLAG_BITS)) : nullptr; }

    bool isOriginal() const { return !isDerived() && !isReference(); }
    bool isDerived() const { return f.der; }
    bool isReference() const { return f.ref; }
    bool isDerivedReference() const { return isDerived() && isReference(); }

    bool isEmpty() const { return _value==0; }
    bool inTransition() const { return f.trans; }
    bool isAllocated() const { return f._ptr==0 && inTransition(); }
    bool isZombie() const { return f._ptr!=0 && inTransition(); }
    bool isUsable() const { return f._ptr!=0 && !inTransition(); }

    CapValue value() const { return _value; }
    CapData data() const { return f._data; }

    bool operator==(Cap other) const { return value() == other.value(); }
    bool operator!=(Cap other) const { return value() != other.value(); }

    Cap asZombie() const { Cap c(_value); c.f.trans=1; return c; }
    Cap asDerived() const { Cap c(_value); c.f.der=1; return c; }
    Cap asReference() const { Cap c(_value); c.f.ref=1; return c; }
    Cap stripReference() const { Cap c(_value); c.f.ref=0; return c; }
    Cap asOriginal() const { Cap c(_value); c.f.ref=0; c.f.der = 0; return c; }
    Cap withPtr(IKernelObject* ptr) const {
      Cap c(_value); c.f._ptr = _packPtr(ptr) & 0x1FFFFFFF; return c;
    }
    Cap withData(CapData data) const { Cap c(_value); c.f._data = data; return c; }
    static Cap asEmpty() { return Cap(); }
    static Cap asAllocated() { Cap c; c.f.trans=1; return c; }

    Cap asReference(CapData data) const { return asReference().withData(data); }
    Cap asReference(IKernelObject* ptr, CapData data) const {
      return asReference().withPtr(ptr).withData(data);
    }

  private:
    struct Fields { // it is a shame that C++11 does still not support unnamed anonymous structs :(
      uint32_t _ptr:29, trans:1, ref:1, der:1;
      CapData _data;
    };
    union {
      CapValue _value;
      Fields f;
    };

    static uint32_t _packPtr(IKernelObject* ptr)
    {
      ASSERT_MSG(!ptr || (kernel2phys(ptr) & FLAG_MASK) == 0,
          "kernel object without propper alignment");
      return (ptr ? kernel2phys(ptr) >> FLAG_BITS : 0);
    }
  };

  template<class T>
  ostream_base<T>& operator<<(ostream_base<T>& out, const Cap& cap)
  {
    out << cap.getPtr();
    if (cap.isUsable()) out << ":usable";
    if (cap.isEmpty()) out << ":empty";
    if (cap.isOriginal() && !cap.isEmpty()) out << ":original";
    if (cap.isDerived()) out << ":derived";
    if (cap.isReference()) out << ":reference";
    if (cap.inTransition()) out << ":in_transition";
    if (cap.isAllocated()) out << ":allocated";
    if (cap.isZombie()) out << ":zombie";
    out << ':';
    out.format(cap.data(), 16);
    return out;
  }

} // namespace mythos
