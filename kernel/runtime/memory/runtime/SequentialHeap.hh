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

#include "util/optional.hh"
#include "util/alignments.hh"
#include "util/FirstFitHeap.hh"
#include "app/mlog.hh"


namespace mythos {

class SpinMutex {
public:
  void lock() {
    while (flag.test_and_set()) { mythos::hwthread_pause(200); }
  }

  void unlock() {
    flag.clear();
  }

  template<typename FUNCTOR>
  void operator<<(FUNCTOR fun) {
    lock();
    fun();
    unlock();
  }

private:
  std::atomic_flag flag;
};

/**
 * Wrapper for FirstFitHeap intrusive.
 */
template<typename T, class A = AlignLine>
class SequentialHeap
{
public:
  typedef T addr_t;
  typedef A Alignment;

  SequentialHeap() {}
  virtual ~SequentialHeap() {}

  size_t getAlignment() const { return heap.getAlignment(); }

  optional<addr_t> alloc(size_t length, size_t alignment) {
    optional<addr_t> res;
    mutex << [&]() { res = heap.alloc(length, alignment); };
    return res;
  }

  void free(addr_t start, size_t length) {
    mutex << [&](){ heap.free(start, length); };
  }

  void addRange(addr_t start, size_t length) {

    mutex << [&]() { 
      MLOG_ERROR(mlog::app, "Add range", DVARhex(start), DVARhex(length)); 
      heap.addRange(start,length); 
    };
  }

private:
  FirstFitHeap<T, A> heap;
  SpinMutex mutex;

};

extern SequentialHeap<uintptr_t> heap;

} // namespace mythos