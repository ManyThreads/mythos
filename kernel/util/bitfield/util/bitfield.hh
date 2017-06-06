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

#include <cstdint>
#include <cstddef>
#include "util/assert.hh"

namespace mythos {

// inspired by http://preshing.com/20150324/safe-bitfields-in-cpp/
#define BITFIELD_DEF(V, NAME)			\
  union NAME {					\
    typedef V value_t;				\
    typedef NAME base_t;			\
    NAME(value_t value) : value(value) {}     \
    operator value_t () const { return value; }	\
    value_t value;
#define BITFIELD_END				\
  };

template<typename value_t, class base_t, size_t START, size_t BITS>
struct UIntField {
  static_assert(START+BITS <= sizeof(value_t)*8, "bits outside of value type");
  constexpr static value_t MASK = value_t(((1ull<<BITS)-1)<<START);
  value_t value;
  base_t operator() (size_t v) const {
    ASSERT(!(v >> BITS));
    return base_t(value_t((value & ~MASK) | ((v<<START) & MASK)));
  }
  void operator= (size_t v) { value = (*this)(v); }
  value_t operator++ () { value_t v = (value&MASK)>>START; value = (*this)(v+1); return v+1; }
  value_t operator++ (int) { value_t v = (value&MASK)>>START; value = (*this)(v+1); return v; }
  operator value_t () const { return (value&MASK)>>START; }
};

template<typename value_t, class base_t, size_t START>
struct BoolField {
  static_assert(START < sizeof(value_t)*8, "bit outside of value type");
  constexpr static value_t MASK = value_t(1ull<<START);
  value_t value;
  base_t operator() (bool v) const {
    return base_t(value_t((value & ~MASK) | ((size_t(v)<<START) & MASK)));
  }
  void operator= (bool v) { value = (*this)(v); }
  operator bool () const { return (value&MASK) >> START; }
};

} // namespace mythos
