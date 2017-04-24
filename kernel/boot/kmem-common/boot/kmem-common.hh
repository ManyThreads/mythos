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

#include "util/RangeSet.hh"
#include "util/PhysPtr.hh"
#include "boot/mlog.hh"
#include "boot/memory-layout.h"
#include "objects/KernelMemory.hh"
#include "util/alignments.hh"

namespace mythos {
  namespace boot {

    extern char KERN_END SYMBOL("KERN_END");

    template<size_t SIZE>
    class KernelMemoryRange
      : public RangeSet<uintptr_t, SIZE>
    {
    public:
      void addStartLength(PhysPtr<void> begin, size_t length) {
        this->add(begin.physint(), begin.physint()+length);
      }

      void removeKernelReserved() {
        reserve(0, 1024*1024, "legacy interrupt vectors etc");
        reserve(Align2M::round_down(LOAD_ADDR), size_t(Align2M::round_up(&KERN_END)), "kernel image");
      }

      void reserve(size_t begin, size_t end, const char* reason) {
        MLOG_DETAIL(mlog::boot, "reserve", DMRANGE(begin, end-begin), "for", reason);
        this->substract(begin, end);
      }

      void addToKM(KernelMemory& km) {
        for (auto& r : *this) {
          MLOG_DETAIL(mlog::boot, "add range", DMRANGE(r.getStart(), r.getSize()), "to kernel memory");
          km.addRange(PhysPtr<void>(r.getStart()), r.getSize());
        }
      }
    };

  } // namespace boot
} // namespace mythos
