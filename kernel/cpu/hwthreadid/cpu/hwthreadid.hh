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
 * Copyright 2014 Randolf Rotta, Maik Krüger, Maximilian Heyne, and contributors, BTU Cottbus-Senftenberg 
 */
#pragma once

#include "cpu/CoreLocal.hh"
#include "util/assert.hh"
#include "boot/memory-layout.h"
#include <cstdint>

namespace mythos {

  namespace cpu {
    typedef uint16_t ThreadID;
    extern CoreLocal<size_t> hwThreadID_ KERNEL_CLM_HOT;
    extern size_t hwThreadCount_;
    extern ThreadID hwThreadIds_[BOOT_MAX_THREADS];

    inline ThreadID hwThreadID() { return ThreadID(hwThreadID_.get()); }

    inline void addHwThreadID(size_t id) {
      ASSERT(hwThreadCount_ < BOOT_MAX_THREADS);
      hwThreadIds_[hwThreadCount_++] = ThreadID(id);
    }

    inline ThreadID enumerateHwThreadID(size_t index) {
      ASSERT(hwThreadCount_ > 0);
      ASSERT(index < hwThreadCount_);
      return hwThreadIds_[index];
    }

    inline size_t hwThreadCount() { return hwThreadCount_; }

    inline void initHWThreadID(size_t id) { hwThreadID_.set(ThreadID(id)); }

  } // namespace cpu
} // namespace mythos
