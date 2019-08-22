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

#include "boot/kmem.hh"
#include "boot/kmem-common.hh"
#include "util/PhysPtr.hh"
#include "boot/mlog.hh"

#include "boot/bootparam.h"

namespace mythos {
  namespace boot {
    
extern struct smp_boot_param *boot_param;
extern unsigned long x86_kernel_phys_base;

void initKernelMemory(KernelMemory& km)
{
  ihk_smp_boot_param_memory_chunk* chunk = reinterpret_cast<ihk_smp_boot_param_memory_chunk*>(reinterpret_cast<uintptr_t>(boot_param) + sizeof(*boot_param) + boot_param->nr_cpus * sizeof(ihk_smp_boot_param_cpu) + boot_param->nr_numa_nodes * sizeof(ihk_smp_boot_param_numa_node));
	MLOG_INFO(mlog::boot, __PRETTY_FUNCTION__, DVAR(chunk), DVAR(boot_param->nr_numa_nodes), DVAR(sizeof(*boot_param)));

  KernelMemoryRange<30> usable_mem;

  for(int n = 0; n < boot_param->nr_memory_chunks; n++){
    MLOG_INFO(mlog::boot, DVARhex(chunk[n].start), DVARhex(chunk[n].end), DVAR(chunk[n].numa_id));
     usable_mem.addStartLength(PhysPtr<void>(chunk[n].start), chunk[n].end-chunk[n].start);	
  }

  //if (ebi.size() > 0) {
    //MLOG_INFO(mlog::boot, "load Linux zero page E820 memory map with size", ebi.size());
    //for (size_t i=0; i < ebi.size(); i++) {
      //auto me = ebi[i];
      //MLOG_DETAIL(mlog::boot, "E820 mmap ", DMRANGE(me.addr.physint(), me.size),
			//(me.isUsable() ? "available" : "reserved"));
      //if (me.isUsable()) usable_mem.addStartLength(me.addr, me.size);
    //}
  //}
  
  //usable_mem.removeKernelReserved();
  //todo: fix this shit
		usable_mem.reserve(0/*Align2M::round_down(x86_kernel_phys_base)*/, (Align2M::round_up(x86_kernel_phys_base + reinterpret_cast<uintptr_t>(&KERN_END))), "kernel image");
  usable_mem.addToKM(km);
  // TODO create array of frame objects for remaining memory above 4GiB
}

  } // namespace boot
} // namespace mythos
