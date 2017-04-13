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
#include "util/MultiBoot.hh"
#include "boot/mlog.hh"

namespace mythos {
  namespace boot {

extern uint64_t _mboot_magic SYMBOL("_mboot_magic");
extern uint64_t _mboot_table SYMBOL("_mboot_table");
extern char KERN_END SYMBOL("KERN_END");

void initKernelMemory(UntypedMemory& um)
{
  KernelMemoryRange<30> usable_mem;
  auto mboot_magic = *physPtr(&_mboot_magic);
  auto mboot_table = *physPtr(&_mboot_table);
  MultiBootInfo mbi(mboot_table);

  // find usable RAM areas and add them to the memory pool
  OOPS_MSG(mboot_magic == MultiBoot::MAGIC, "No MultiBoot memory map found :(");
  MLOG_DETAIL(mlog::boot, "multiboot table: ", DVARhex(mboot_magic), DVARhex(mboot_table));
  if (mboot_magic == MultiBoot::MAGIC) {
    mbi.foreach([&usable_mem](MultiBootInfo::MMapEntry const* me) {
      MLOG_DETAIL(mlog::boot, "MB mmap ", DMRANGE(me->getAddress().physint(), me->getSize()),
       (me->isAvailable() ? " available " : (me->isReserved() ? " reserved " : "NA")));
      if (me->isAvailable())
	      usable_mem.addStartLength(me->getAddress(), me->getSize());
    });
  }

  usable_mem.removeKernelReserved();
  usable_mem.addToUM(um);
  /// @todo create array of frame objects for remaining memory above 4GiB
}

  } // namespace boot
} // namespace mythos
