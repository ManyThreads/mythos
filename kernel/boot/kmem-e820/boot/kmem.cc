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
#include "util/E820.hh"
#include "boot/mlog.hh"

namespace mythos {
  namespace boot {
    
void initKernelMemory(UntypedMemory& um)
{
  KernelMemoryRange<30> usable_mem;
  E820Info ebi;
  OOPS_MSG(ebi.size()>0, "No Linux zero page E820 memory map found! :(");
  if (ebi.size() > 0) {
    MLOG_INFO(mlog::boot, "load Linux zero page E820 memory map with size", ebi.size());
    for (size_t i=0; i < ebi.size(); i++) {
      auto me = ebi[i];
      MLOG_DETAIL(mlog::boot, "E820 mmap ", DMRANGE(me.addr.physint(), me.size),
			(me.isUsable() ? "available" : "reserved"));
      if (me.isUsable()) usable_mem.addStartLength(me.addr, me.size);
    }
  }
  
  usable_mem.removeKernelReserved();
  usable_mem.addToUM(um);
  // TODO create array of frame objects for remaining memory above 4GiB
}

  } // namespace boot
} // namespace mythos
