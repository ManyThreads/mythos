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
 * Copyright 2019 Philipp Gypser and contributors, BTU Cottbus-Senftenberg
 */

#include "boot/apboot.hh"
#include "util/assert.hh"
#include "cpu/PIC.hh"
#include "cpu/LAPIC.hh"
#include "cpu/ctrlregs.hh"
#include "boot/memory-layout.h"
#include "boot/DeployHWThread.hh"
#include "util/PhysPtr.hh"
#include "boot/mlog.hh"
#include "boot/bootparam.h"
#include "cpu/hwthreadid.hh"
#include "boot/ihk-entry.hh"

extern char _dbg_trampoline;
extern char _dbg_trampoline_end;

namespace mythos {
  namespace boot {

int ihk_get_nr_cores(void) { return boot_param->nr_cpus; }

ihk_smp_boot_param_cpu* get_param_cpu(int i)
{
  return reinterpret_cast<ihk_smp_boot_param_cpu*>(reinterpret_cast<uintptr_t>(boot_param) + sizeof(*boot_param) + i * sizeof(ihk_smp_boot_param_cpu));
}

int ihk_get_apicid(int i)
{
  return boot_param->ihk_ikc_irq_apicids[get_param_cpu(i)->linux_cpu_id];
}

/** basic cpu configuration, indexed by the logical thread ID. */
DeployHWThread ap_config[MYTHOS_MAX_THREADS];

/** mapping from apicID to the thread's configuration.  This is used
 * during boot to get the right thread configuration while the
 * core-local memory not yet available. It is indexed by the initial
 * apicID, which was gathered via the cpuid instruction.
 */
DeployHWThread* ap_apic2config[MYTHOS_MAX_APICID];

NORETURN extern void start_ap64(size_t reason) SYMBOL("_start_ap64");
NORETURN extern void start_ap64_pregdt(size_t reason) SYMBOL("_start_ap64_pregdt");
NORETURN extern void start_ap64_trampoline() SYMBOL("_start_ap64_trampoline");
NORETURN extern void start_ap64_loop() SYMBOL("_start_ap64_loop");
NORETURN extern void entry_ap(size_t apicID, size_t reason) SYMBOL("entry_ap");

struct PACKED dbg_t {
  uint64_t jump16;
  uint64_t table32;
  uint64_t pml4;
  uint64_t startap;
  uint32_t jump32;
  uint16_t cs32;
  uint16_t gdt32_size;
  uint32_t gdt32_ptr;
};

void printDbg()
{
  auto tr = reinterpret_cast<dbg_t*>(IHK_TRAMPOLINE_ADDR);
  MLOG_DETAIL(mlog::boot,
    DVARhex(IHK_TRAMPOLINE_ADDR),
    DVARhex(tr),
    DVARhex(tr->jump16),
    DVARhex(tr->table32),
    DVARhex(tr->pml4),
    DVARhex(tr->startap),
    DVARhex(tr->jump32),
    DVARhex(tr->cs32),
    DVARhex(tr->gdt32_size),
    DVARhex(tr->gdt32_ptr));
}


bool apboot_thread(size_t apicID, size_t reason) {
  return ap_apic2config[apicID]->initThread(reason);
}

NORETURN void apboot()
{
  // read acpi topology, then initialise HWThread objects
  cpu::hwThreadCount = ihk_get_nr_cores();
  ASSERT(cpu::getNumThreads() < MYTHOS_MAX_THREADS);
  MLOG_WARN(mlog::boot, DVAR(cpu::getNumThreads()), DVAR(ihk_get_nr_cores()));

  for (cpu::ThreadID id=0; id<cpu::getNumThreads(); id++) {
    MLOG_INFO(mlog::boot, DVAR(id),
        DVAR(get_param_cpu(id)->numa_id),
        DVAR(get_param_cpu(id)->hw_id),
        DVAR(get_param_cpu(id)->linux_cpu_id),
        DVAR(get_param_cpu(id)->ikc_cpu));
    auto apicID = ihk_get_apicid(id);
    ASSERT(apicID < MYTHOS_MAX_APICID);
    ap_config[id].prepare(id, cpu::ApicID(apicID));
    ap_apic2config[apicID] = &ap_config[id];
  }

  auto bsp_apic_id = x86::initialApicID();
  MLOG_DETAIL(mlog::boot, DVAR(bsp_apic_id));

  DeployHWThread::prepareBSP();
  MLOG_DETAIL(mlog::boot, "Init Trampoline ", DVARhex(ap_trampoline));
  mapTrampoline(ap_trampoline);

  auto tr = reinterpret_cast<trampoline_t*>(IHK_TRAMPOLINE_ADDR);
  MLOG_DETAIL(mlog::boot, "initial values",
    DVARhex(tr->jump_intr),
    DVARhex(tr->header_pgtbl),
    DVARhex(tr->header_load),
    DVARhex(tr->stack_ptr),
    DVARhex(tr->notify_addr));
  ASSERT_MSG(tr->jump_intr == 0x26ebull, "expected 'jmp 0x28' at begin of trampoline header");

  trampoline_t old_tr = *tr;

  // TODO: proper device mem pointer
  auto  trampoline_phys_high = reinterpret_cast<uint16_t volatile*>(LOW_MEM_ADDR + 0x469ul);
  auto  trampoline_phys_low = reinterpret_cast<uint16_t volatile*>(LOW_MEM_ADDR + 0x467ul);
  ASSERT(*trampoline_phys_high == uint16_t(ap_trampoline >> 4));
  ASSERT(*trampoline_phys_low == uint16_t(ap_trampoline & 0xf));

  mythos::lapic.init(); // does check for lapic presence and activation via MSRs

  // loop through the assigned cores and boot one after another
  for (cpu::ThreadID id=0; id<cpu::getNumThreads(); id++) {
    auto apicID = cpu::ApicID(ihk_get_apicid(id));
    if (apicID != bsp_apic_id) {

      // TODO: proper device mem pointer
      memcpy(
          reinterpret_cast<void*>(IHK_TRAMPOLINE_ADDR),
          &_dbg_trampoline,
          size_t(&_dbg_trampoline_end-&_dbg_trampoline));

      auto t = reinterpret_cast<dbg_t*>(IHK_TRAMPOLINE_ADDR);
      t->table32 = old_tr.header_pgtbl;
      t->pml4 = virt_to_phys(pml4_table);
      t->startap = reinterpret_cast<uint64_t>(&start_ap64_pregdt);

      asm volatile("wbinvd"::: "memory");
      printDbg();

      auto before = *reinterpret_cast<volatile uint32_t*>(IHK_TRAMPOLINE_ADDR);
      asm volatile ("" ::: "memory");
      //MLOG_ERROR(mlog::boot, "before send", DVARhex(before));
      mythos::lapic.startup(apicID, ap_trampoline);
      //MLOG_ERROR(mlog::boot, "loop for change ...");
      while(1) {
        asm volatile ("" ::: "memory");
        uint32_t after = *reinterpret_cast<volatile uint32_t*>(IHK_TRAMPOLINE_ADDR);
        asm volatile ("" ::: "memory");
        if (after != before) {
	  //MLOG_ERROR(mlog::boot, DVARhex(after));
          printDbg();
          before = after;
          if (after == 7) break;
        }
      }
    } else {
      MLOG_DETAIL(mlog::boot, "Skipped BSP in startup", DVAR(bsp_apic_id));
    }
  }

  MLOG_ERROR(mlog::boot, "set status");
  boot_param->status = 2; // this enables output
  asm volatile("" ::: "memory");
  MLOG_ERROR(mlog::boot, "set status done ");
  
  start_ap64_pregdt(0); // will never return from here!
  while(1);
}

  } // namespace boot
} // namespace mythos
