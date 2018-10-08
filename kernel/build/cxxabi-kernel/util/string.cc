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

#include <cstddef> // for size_t

extern "C" void* memcpy(void* dst, void const* src, size_t count) {
    char* d = reinterpret_cast<char*>(dst);
    char const* s = reinterpret_cast<char const*>(src);
    for(size_t index = 0; index < count; index++){
      d[index] = s[index];
    }
    return dst;
}

extern "C" void* memset(void* dst, int value, size_t count) {
    char* d = reinterpret_cast<char*>(dst);
    char s = (char)value;
    for(size_t index = 0; index < count; index++){
      d[index] = s;
    }
    return dst;
}

extern "C" size_t strlen(const char *s) {
    if (s == nullptr) return 0;
    size_t len=0;
    while (s[len] != 0) len++;
    return len;
}

extern "C" int strcmp(const char *s1, const char *s2)
{
  while (*s1 && *s2 && *s1 == *s2) {
    s1++;
    s2++;
  }
  return *s1 - *s2;
}
