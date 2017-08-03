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

#include "boot/mlog.hh"
#include "cpu/MLogSinkSerial.hh"
#include "boot/memory-layout.h"
#include <new>

namespace mythos {
  namespace boot {

    alignas(MLogSinkSerial) char mlogsink[sizeof(MLogSinkSerial)];

    void initMLog()
    {
      // need constructor for vtable before global constructors were called
      auto s = new(mlogsink) MLogSinkSerial(*(unsigned short*)(0x400+KERNELMEM_ADDR));
      mlog::sink = s; // now logging is working
      mlog::boot.setName("boot"); // because constructor is still not called
    }
  } // namespace boot
} // namespace mythos


namespace mlog {
  Logger<MLOG_BOOT> boot("boot");
} // namespace mlog
