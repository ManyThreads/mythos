/* -*- mode:C++; c-file-style:"bsd"; -*- */
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
 * Copyright 2013 Randolf Rotta, Maik Krüger, and contributors, BTU Cottbus-Senftenberg
 */
#pragma once

#include <cstddef>

namespace mythos {

template<typename T, size_t M>
class VectorMax
{
public:
  typedef T value_type;

  VectorMax() : psize(0) {}
  VectorMax(size_t size) : psize(size) {}

  size_t size() const { return psize; }
  size_t capacity() const { return M; }
  void resize(size_t psize) { this->psize = psize; }

  T& get(size_t i) { return data[i]; }
  T const& get(size_t i) const { return data[i]; }

  T& operator[](size_t i) { return data[i]; }
  T const& operator[](size_t i) const { return data[i]; }

  T* begin() { return data; }
  T* end() { return data+psize; }

  void push_back(T const& e) { data[psize++] = e; }
  void pop_back() { psize--; }
  T const& back() const { return *(data+psize-1); }
  T& back() { return *(data+psize-1); }

protected:
  size_t psize;
  T data[M];
};

} // namespace mythos
