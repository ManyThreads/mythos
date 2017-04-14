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

namespace mythos {

template<class T, size_t CAPACITY>
class BacktrackBuffer
{
public:

  BacktrackBuffer() : _head(0), _tail(0), _overflow(false) {}

  bool empty() { return _head == _tail; }
  bool overflown() { return _overflow; }

  T& back() { return _buf[_tail]; }

  void push_back(const T& val)
  {
    _buf[inc(_tail)] = val;
    if (_head == _tail) {
      _overflow = true;
      inc(_head);
    }
  }

  T pop_back()
  {
    ASSERT(!empty());
    T result;
    dec(_tail);
  }

private:

  size_t& inc(size_t& index)
  {
    ++index;
    index %= CAPACITY;
    return index;
  }

  size_t& dec(size_t& index)
  {
    if (!index) {
      index = CAPACITY-1;
    } else {
      --index;
    }
    return index;
  }

  size_t _head;
  size_t _tail;
  bool _overflow;
  T _buf[CAPACITY];

};

}
