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
 * Copyright 2014 Philipp Gypser, BTU Cottbus-Senftenberg
 */
#pragma once

#include <util/compiler.hh>
#include <cstddef>
#include <cstdint>

namespace mythos {

extern  uint64_t tscdelay_MHz;

// Requires a compiler flag in order to make test-compilation outside
// of KNC toolchain possible.  Done in this strange way to remove the
// "unused parameter" warning.
#ifdef __MIC__
  inline void hwthread_pause(size_t cycles=10) {
    asm volatile("delay %0" : : "r"(cycles) :);
  }

  inline void hwthread_wait(size_t usec){
    unsigned low,high;
    asm volatile("rdtsc" : "=a" (low), "=d" (high));
    uint64_t start = low | uint64_t(high) << 32;
    uint64_t nticks = usec*tscdelay_MHz;
    while (true) {
        asm volatile("rdtsc" : "=a" (low), "=d" (high));
        uint64_t now = low | uint64_t(high) << 32;
        if (now - start >= nticks) break;
        asm volatile("rep; nop" ::: "memory");
    }
  }
#else
  inline void hwthread_pause() {}
  inline void hwthread_pause(size_t) {}
  inline void hwthread_wait(size_t) {}
#endif

  inline void hwthread_pollpause() { hwthread_pause(300); }

  NORETURN inline void sleep_infinitely()
  {
    while(true) {
      asm("hlt");
    };
  }

} // namespace mythos
