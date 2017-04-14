/* -*- mode:C++; indent-tabs-mode:nil; -*- */
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
 * Copyright 2016 Randolf Rotta, Robert Kuban, and contributors, BTU Cottbus-Senftenberg
 */
#include "boot/pagetables.hh"

#define table_to_phys_addr(t, subtable) \
  (uint64_t(t) + (subtable)*PAGETABLE_SIZE - VIRT_ADDR)

/*
dnl Define for-loop: http://mbreen.com/m4.html
define(`forloop',`ifelse($#,0,``$0'',`ifelse(eval($2<=$3),1, `pushdef(`$1',$2)$4`'popdef(`$1')$0(`$1',incr($2),$3,`$4')')')')dnl
define(`NL',`
')dnl
*/

namespace mythos {
namespace boot {

ALIGN_4K uint64_t devices_pml1[] = {
  INVALID, // dummy entry to separate from the kernel memory
  // vaddr 0xffff8001 00001000 (LAPIC_ADDR)
  INVALID, // LAPIC mapping
forloop(`i',2,511,`  INVALID, // i'NL)dnl

  // kernel stacks: 16KiB for each hardware thread with a free page inbetween, max 340 stacks
  forloop(`i',0,511,`  INVALID, // i'NL)dnl
  forloop(`i',0,511,`  INVALID, // i'NL)dnl
};

ALIGN_4K uint64_t devices_pml2[] = {
  // vaddr 0xffff8001 00000000 (DEVICES_ADDR)
  PRESENT + WRITE + USER + ACCESSED + table_to_phys_addr(devices_pml1,0), // LAPIC
  // vaddr 0xffff8001 00200000 (KERNELSTACKS_ADDR)
  PRESENT + WRITE + USER + ACCESSED + table_to_phys_addr(devices_pml1,1), // kernel stacks
  PRESENT + WRITE + USER + ACCESSED + table_to_phys_addr(devices_pml1,2), // kernel stacks
  // vaddr 0xffff8001 00600000 (usable for additional hardware like XeonPhi's MMIO registers)
forloop(`i',3,511,`  INVALID, // i'NL)dnl
};

  /// @todo add more functions like initKernelStack, but compute index from logical address, including  assertions.

ALIGN_4K uint64_t image_pml2[] = {
forloop(`i',0,7,`  INVALID, // i'NL)dnl
forloop(`i',0,1015,`  PML2_BASE + i*PML2_PAGESIZE,'NL)dnl
};

ALIGN_4K uint64_t pml2_tables[] = {
forloop(`i',0,2047,`  PML2_BASE + i*PML2_PAGESIZE,'NL)dnl
};

ALIGN_4K uint64_t pml3_table[] = {
  // 4GiB direct mapped area and page tables for memory mapped devices
  // vaddr 0x00000000 00000000 and 0xffff8000 00000000 (KERNELMEM_ADDR)
forloop(`i',0,3,`  PML3_BASE + table_to_phys_addr(pml2_tables,i),'NL)dnl
  PML3_BASE + table_to_phys_addr(devices_pml2,0),
forloop(`i',5,511,`  INVALID, // i'NL)dnl

  // 2GiB kernel image area
forloop(`i',0,509,`  INVALID, // i'NL)dnl
  // vaddr 0xffffffff 81000000 (VIRT_ADDR)
forloop(`i',0,1,`  PML3_BASE + table_to_phys_addr(image_pml2,i),'NL)dnl
};

ALIGN_4K uint64_t pml4_table[] = {
  // boot variant with lower half such that the init segment is usable
  // vaddr 0x00000000 00000000
  PML4_BASE + table_to_phys_addr(pml3_table,0), // lower half
forloop(`i',1,255,`  INVALID, // i'NL)dnl
  PML4_BASE + table_to_phys_addr(pml3_table,0), // upper half direct mapped area
forloop(`i',257,510,`  INVALID, // i'NL)dnl
  PML4_BASE + table_to_phys_addr(pml3_table,1), // upper half kernel image area

  // final variant without lower half
  // vaddr 0x00000000 00000000
forloop(`i',0,255,`  INVALID, // i'NL)dnl
  PML4_BASE + table_to_phys_addr(pml3_table,0), // upper half direct mapped area
forloop(`i',257,510,`  INVALID, // i'NL)dnl
  PML4_BASE + table_to_phys_addr(pml3_table,1), // upper half kernel image area
};

#undef table_to_phys_addr

} // boot
} // mythos
