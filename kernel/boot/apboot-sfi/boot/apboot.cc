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
#include "cpu/LAPIC.hh"
#include "cpu/ctrlregs.hh"
#include "boot/memory-layout.h"
#include "util/SFI.hh"
#include "boot/DeployHWThread.hh"
#include "util/MPApicTopology.hh"
#include "cpu/IOApic.hh"

namespace mythos {
  namespace boot {

/** basic cpu configuration, indexed by the logical thread ID. */
DeployHWThread ap_config[MYTHOS_MAX_THREADS];

/** mapping from apicID to the thread's configuration.  This is used
 * during boot to get the right thread configuration while the
 * core-local memory not yet available. It is indexed by the initial
 * apicID, which was gathered via the cpuid instruction.
 */
DeployHWThread* ap_apic2config[MYTHOS_MAX_APICID];
void apboot_thread(size_t apicID) { ap_apic2config[apicID]->initThread(); }

NORETURN extern void start_ap64(size_t reason) SYMBOL("_start_ap64");

NORETURN void apboot()
{
  // read acpi topology, then initialise HWThread objects
  SFIInfo topo;
  cpu::hwThreadCount = topo.numThreads();
  ASSERT(cpu::getNumThreads() < MYTHOS_MAX_THREADS);
  for (cpu::ThreadID id=0; id<cpu::getNumThreads(); id++) {
    ASSERT(topo.threadID(id)<MYTHOS_MAX_APICID);
    ap_config[id].prepare(id, cpu::ApicID(topo.threadID(id)));
    ap_apic2config[topo.threadID(id)] = &ap_config[id];
  }

  auto apics = topo.getApics();
  for (uint64_t i = 0; i < topo.numApics(); i++) {
    uint64_t addr = apics[i]->apicID[0];
    auto offs = addr - MMIO_PHYS;
    ioapic.init(MMIO_ADDR + offs);
  }

  DeployHWThread::prepareBSP();

  // broadcast Startup IPI
  initAPTrampoline(0x40000);
  mythos::x86::enableApic(); // just to be sure it is enabled
  mythos::lapic.init();
  mythos::lapic.broadcastInitIPIEdge();
  mythos::lapic.broadcastStartupIPI(0x40000);

  // switch to BSP's stack here
  start_ap64(0); // will never return from here!
}

  } // namespace boot
} // namespace mythos
