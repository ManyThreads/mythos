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

#include <cstddef> // for size_t

namespace mythos {

  inline void* memcpy(void* dst, void const* src, size_t count) {
    auto d = reinterpret_cast<char*>(dst);
    auto s = reinterpret_cast<char const*>(src);
    for(size_t index = 0; index < count; index++){
      d[index] = s[index];
    }
    return dst;
  }

  inline void* memset(void* dst, char value, size_t count) {
    auto d = reinterpret_cast<char*>(dst);
    for(size_t index = 0; index < count; index++) d[index] = value;
    return dst;
  }

  inline int strcmp(const char *s1, const char *s2)
  {
    while (*s1 && *s2 && *s1 == *s2) {
      s1++;
      s2++;
    }
    return *s1 - *s2;
  }

  inline size_t strlen(const char *s) {
    if (s == nullptr) return 0;
    size_t len=0;
    while (s[len] != 0) len++;
    return len;
  }

  template<size_t N>
  inline size_t strlen(const char[N]) { return N; }

  inline int memcmp (const void* ptr1, const void* ptr2, size_t num)
  {
    auto p1 = reinterpret_cast<const unsigned char*>(ptr1);
    auto p2 = reinterpret_cast<const unsigned char*>(ptr2);

    for (size_t i=0; i<num; i++) {
      if (p1[i] != p2[i]) return p1[i] < p2[i] ? -1 : 1;
    }
    return 0;
  }

} // namespace mythos
