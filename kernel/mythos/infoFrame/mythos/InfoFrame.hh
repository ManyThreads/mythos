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
 * Copyright 2020 Philipp Gypser, BTU Cottbus-Senftenberg
 */
#pragma once

#include "mythos/InvocationBuf.hh"
#include "mythos/init.hh"

namespace mythos {

constexpr uint64_t PS_PER_TSC_DEFAULT = 0x180;
constexpr size_t MAX_IB = 4096;

class InfoFrame{
  public:
    InfoFrame()
      : psPerTsc(PS_PER_TSC_DEFAULT)
      , numThreads(1)
    {}

    InvocationBuf* getInvocationBuf() {return &ib[0]; }
    InvocationBuf* getInvocationBuf(size_t i) {return &ib[i]; }
    size_t getIbOffset(size_t i){
      return reinterpret_cast<uintptr_t>(&ib[i]) - reinterpret_cast<uintptr_t>(this);
    }

    uint64_t getPsPerTSC() { return psPerTsc; }
    size_t getNumThreads() { return numThreads; }
    uintptr_t getInfoEnd () { return reinterpret_cast<uintptr_t>(this) + sizeof(InfoFrame); }

    InvocationBuf ib[MAX_IB]; // needs to be the first member (see Initloader::createPortal)
    uint64_t psPerTsc; // picoseconds per time stamp counter
    size_t numThreads; // number of hardware threads available in the system
};

} // namespace mythos
