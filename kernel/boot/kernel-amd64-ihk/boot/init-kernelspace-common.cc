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

#include "boot/DeployKernelSpace.hh"
#include "boot/pagetables.hh"
#include "boot/memory-layout.h"
#include "boot/mlog.hh"

extern char KERN_ROEND;
extern char KERN_END;

//extern uint64_t _mboot_table;

namespace mythos {
  namespace boot {

void initKernelSpaceCommon()
{
  // remove rw access from 0 to LOAD_ADDR from image
  //static_assert(LOAD_ADDR == PML2_PAGESIZE, "failed assumption about kernel layout");
  //static_assert(VIRT_ADDR == 0xffffffff81000000, "failed assumption about kernel layout");
  //image_pml2[8] &= ~PRESENT;
  // remove write access on kernel code from kernelmem and image
  // phys address range is from LOAD_ADDR to KERN_ROEND
  //for (unsigned i=LOAD_ADDR/PML2_PAGESIZE;
       //i<reinterpret_cast<uintptr_t>(&KERN_ROEND)/PML2_PAGESIZE;
       //i++) {
    //image_pml2[8+i] &= ~WRITE;
    //pml2_tables[i] &= ~WRITE;
  //}
  // rw but no execute access on kernel data in image from KERN_ROEND to KERN_END
  /// @todo set NOEXECUTE
  // remove access (read and write) from remaining space behind KERN_END in the image
  // first 8 entries are already invalid (see pagetables.cc.m4)
  //for (auto i = 8+reinterpret_cast<uintptr_t>(&KERN_END)/PML2_PAGESIZE; i<1024; i++) {
    //image_pml2[i] &= ~PRESENT;
  //}
}

void loadKernelSpace()
{
  asm volatile("mov %0,%%cr3": : "r" (table_to_phys_addr(pml4_table,0)));
}

void mapLapic(uintptr_t phys)
{
  devices_pml1[1] = CD + PRESENT + WRITE + ACCESSED + DIRTY + GLOBAL + phys;
}

void mapIOApic(uintptr_t phys) {
  MLOG_ERROR(mlog::boot, "map ioapic", DVARhex(phys));
  devices_pml1[2] = CD + PRESENT + WRITE + ACCESSED + DIRTY + GLOBAL + phys;
}

void mapTrampoline(uintptr_t phys) {
  MLOG_ERROR(mlog::boot, "map trampoline", DVARhex(phys));
  devices_pml1[3] = PRESENT + WRITE + ACCESSED + DIRTY + GLOBAL + phys;
}

uintptr_t initKernelStack(size_t idx, uintptr_t paddr)
{
  static_assert(CORE_STACK_SIZE == 2*4096, "failed assumption about kernel layout");
  devices_pml1[512 + 3*idx + 1] = PRESENT + WRITE + ACCESSED + DIRTY + GLOBAL + paddr;
  devices_pml1[512 + 3*idx + 2] = PRESENT + WRITE + ACCESSED + DIRTY + GLOBAL + paddr;
  return KERNELSTACKS_ADDR + 4096*(3*idx + 3); // return uppermost vaddr of the stack
}

  } // namespace boot
} // namespace mythos
