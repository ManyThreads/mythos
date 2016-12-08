/* -*- mode:C++; -*- */
/* MyThOS: The Many-Threads Operating System
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

#include "boot/kmem.hh"
#include "boot/kmem-common.hh"
#include "util/PhysPtr.hh"
#include "boot/SFI.hh"
#include "boot/mlog.hh"

namespace mythos {
namespace boot {

extern char KERN_END SYMBOL("KERN_END");

void initKernelMemory(UntypedMemory& um)
{
  KernelMemoryRange<30> usable_mem;
  SFIInfo info;
  OOPS_MSG(info.hasSyst() && info.hasMmap(), "No memory map found :(");
  if (info.hasSyst() && info.hasMmap()) {
    SFI::Mmap& mmap = *info.getMmap();
    mlog::boot.detail("parsing SFI System Table", DVAR(&mmap));

    for(unsigned j=0; j<mmap.size(); ++j) {
      auto me = mmap[j];
      mlog::boot.detail("MB mmap ", DMRANGE(me.getAddress().physint(), me.getSize()),
          (me.type == SFI::Mmap::ConventionalMemory ? " available " : "reserved"));
      if (me.type == SFI::Mmap::ConventionalMemory)
        usable_mem.addStartLength(me.getAddress(), me.getSize());
    }
  }

  usable_mem.removeKernelReserved();
  usable_mem.addToUM(um);
  // TODO create array of frame objects for remaining memory above 4GiB
}

} // namespace boot
} // namespace mythos
