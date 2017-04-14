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
 * Copyright 2014 Randolf Rotta, Maik Krüger, and contributors, BTU Cottbus-Senftenberg
 */
#pragma once

#include <cstdint>

namespace mythos {

  /** fnv-1a hash, useful for debugging
   * http://www.isthe.com/chongo/tech/comp/fnv/
   */
  template<class T, T BASE, T PRIME>
  T fnv1a(void* start, size_t length)
  {
    T hash = BASE;
    auto buf = reinterpret_cast<uint8_t*>(start);
    for (auto it = buf; it < buf+length; ++it) {
      hash ^= *it;
      hash *= PRIME;
    }
    return hash;
  }

  /** fnv-1a hash, useful for debugging
   * http://www.isthe.com/chongo/tech/comp/fnv/
   */
  inline uint32_t hash32(void* start, size_t length)
  {
    return fnv1a<uint32_t, 2166136261u, 16777619u>(start, length);
  }

  /** fnv-1a hash, useful for debugging
   * http://www.isthe.com/chongo/tech/comp/fnv/
   */
  inline uint16_t hash16(void* start, size_t length)
  {
    auto hash = hash32(start, length);
    return uint16_t((hash >> 16) ^ hash);
  }

  /** fnv-1a hash, useful for debugging
   * http://www.isthe.com/chongo/tech/comp/fnv/
   */
  inline uint64_t hash64(void* start, size_t length)
  {
    return fnv1a<uint64_t, 14695981039346656037u, 2166136261u>(start, length);
  }

} // namespace mythos
