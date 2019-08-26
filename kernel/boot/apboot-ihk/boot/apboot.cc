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

#include "boot/apboot.hh"
#include "util/assert.hh"
#include "cpu/PIC.hh"
#include "cpu/LAPIC.hh"
#include "cpu/IOApic.hh"
#include "cpu/ctrlregs.hh"
#include "boot/memory-layout.h"
#include "boot/DeployHWThread.hh"
#include "util/PhysPtr.hh"
#include "boot/mlog.hh"
#include "boot/bootparam.h"
#include "cpu/hwthreadid.hh"
#include "boot/ihk-entry.hh"

namespace mythos {
  namespace boot {


int ihk_get_nr_cores(void)
{
	return boot_param->nr_cpus;
}

ihk_smp_boot_param_cpu* get_param_cpu(int i){
	return reinterpret_cast<ihk_smp_boot_param_cpu*>(reinterpret_cast<uintptr_t>(boot_param) + sizeof(*boot_param) + i * sizeof(ihk_smp_boot_param_cpu));
}

int ihk_get_apicid(int i) {
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

void apboot_thread(size_t apicID)
{ 
  MLOG_DETAIL(mlog::boot, "ap_boot_thread");
  //while(1);
	ap_apic2config[apicID]->initThread();
}

NORETURN void apboot() {
  // read acpi topology, then initialise HWThread objects
  //MPApicTopology topo;
  cpu::hwThreadCount = ihk_get_nr_cores();
  ASSERT(cpu::getNumThreads() < MYTHOS_MAX_THREADS);
	MLOG_INFO(mlog::boot, DVAR(cpu::getNumThreads()), DVAR(ihk_get_nr_cores()));
 
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
  // test: we leave everything as it is, but jump to _start_ap64_loop
  // just loop -> F390EBFC
  //tr->jump_intr = 0xFCEB90F3ull;

  mythos::cpu::disablePIC(); // <- maybe don't do this, should only concern IOAPIC
  //mythos::x86::enableApic(); // just to be sure it is enabled
  mythos::lapic.init(); // maybe do the enabling check here?

  for (cpu::ThreadID id=0; id<cpu::getNumThreads(); id++) {
    auto apicID = cpu::ApicID(ihk_get_apicid(id));
    if (apicID != bsp_apic_id) {

      //tr->header_pgtbl = *pml4_table;
      tr->header_load = reinterpret_cast<uint64_t>(&start_ap64_loop);
      tr->stack_ptr = DeployHWThread::stacks[apicID];
      // fails before longjmp, maybe init is not correct?
      auto patch = reinterpret_cast<uint64_t*>(IHK_TRAMPOLINE_ADDR+0x62); // <- this works
      //auto patch = reinterpret_cast<uint64_t*>(IHK_TRAMPOLINE_ADDR+0x70);
      *patch = 0xFCEB90F3ull;
      asm volatile("wbinvd"::: "memory");

      //asm volatile("cli"::: "memory");
      MLOG_INFO(mlog::boot, "Send Init IPI", DVAR(apicID));
      mythos::lapic.sendInitIPIEdge(apicID);
      MLOG_INFO(mlog::boot, "Send SIPI", DVAR(apicID));
      mythos::lapic.sendStartupIPI(apicID, ap_trampoline);
      //asm volatile("sti"::: "memory");
      //while(1) hwthread_pause();
      break; // one is enough for today
    } else {
      MLOG_DETAIL(mlog::boot, "Skipped BSP in startup", DVAR(bsp_apic_id));
    }
  }

  //// switch to BSP's stack here
  //entry_ap(0,0);
  start_ap64_pregdt(0); // will never return from here!
  //start_ap64(0); // will never return from here!
  while(1);
}

  } // namespace boot
} // namespace mythos
