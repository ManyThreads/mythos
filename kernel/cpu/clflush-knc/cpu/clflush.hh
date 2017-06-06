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
 * Copyright 2016 Martin Messer, BTU Cottbus-Senftenberg
 */
#pragma once

namespace mythos {
  namespace cpu {
  enum Const{ CACHELINESIZE=64};

// Requires a compiler flag in order to make test-compilation outside
// of KNC toolchain possible.  Done in this strange way to remove the
// "unused parameter" warning.
#ifdef __MIC__
  inline void clflush(void* addr){
    // both needed according to http://htor.inf.ethz.ch/publications/img/ramos-hoefler-cc-modeling.pdf
    asm volatile("clevict0 (%0)"::"r"(addr)); // evict from local L1 cache
    asm volatile("clevict1 (%0)"::"r"(addr)); // evict from local L2 cache
  }
#else
  inline void clflush(void* addr){
    asm volatile("clflush (%0)"::"r"(addr));
  }
#endif

  inline void wbinvd(){
    asm volatile("wbinvd" :::"memory");
  }

  } // namespace cpu
} // namespace mythos
