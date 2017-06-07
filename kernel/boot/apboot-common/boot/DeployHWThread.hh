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
 * Copyright 2014 Randolf Rotta, Maik Krüger, and contributors, BTU Cottbus-Senftenberg
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include "plugins/Plugin.hh"
#include "util/compiler.hh"
#include "util/assert.hh"
#include "cpu/GdtAmd64.hh"
#include "cpu/IdtAmd64.hh"
#include "cpu/CoreLocal.hh"
#include "cpu/hwthreadid.hh"
#include "cpu/kernel_entry.hh"
#include "async/Place.hh"
#include "objects/DeleteBroadcast.hh"
#include "objects/SchedulingContext.hh"
#include "boot/memory-layout.h"
#include "boot/DeployKernelSpace.hh"
#include "boot/mlog.hh"

namespace mythos {
  namespace boot {

    void initAPTrampoline(size_t startIP);

    extern SchedulingContext schedulers[BOOT_MAX_THREADS];
    extern CoreLocal<SchedulingContext*> localScheduler KERNEL_CLM;

    SchedulingContext& getScheduler(size_t index) { return schedulers[index]; }
    SchedulingContext& getLocalScheduler() { return *localScheduler; }

struct DeployHWThread
{
  ALIGN_4K static char stackspace[CORE_STACK_SIZE*BOOT_MAX_THREADS];
  ALIGN_4K static char nmistackspace[NMI_STACK_SIZE*BOOT_MAX_THREADS];

  /** pointers to the AP stacks indexed by APIC ID */
  static uintptr_t stacks[BOOT_MAX_THREADS] SYMBOL("ap_startup_stacks");

  static void prepareBSP(size_t startIP) {
    idt.init();
    initAPTrampoline(startIP);
    DeleteBroadcast::init();
    Plugin::initPluginsGlobal();
  }

  void prepare(size_t apicID) {
    this->firstboot = true;
    PANIC_MSG(apicID<BOOT_MAX_THREADS, "unexpectedly large apicID");
    cpu::addHwThreadID(apicID);
    gdt.init();
    stacks[apicID] = initKernelStack(apicID, uintptr_t(stackspace)+apicID*CORE_STACK_SIZE-VIRT_ADDR);
    tss_kernel.sp[0] = stacks[apicID];
    tss_kernel.ist[1] = uintptr_t(nmistackspace)+apicID*NMI_STACK_SIZE+NMI_STACK_SIZE;
    gdt.tss_kernel.set(&tss_kernel);
    gdt.kernel_gs.setBaseAddress(uint32_t(KernelCLM::getOffset(apicID)));
    async::preparePlace(apicID);

    MLOG_DETAIL(mlog::boot, "  mapped kernel stack", DVAR(apicID),
                      (void*)stacks[apicID], (void*)(uintptr_t(stackspace)+apicID*CORE_STACK_SIZE-VIRT_ADDR));
    MLOG_DETAIL(mlog::boot, "  nmi stack", DVAR(apicID),
                      (void*)tss_kernel.ist[1]);
    MLOG_DETAIL(mlog::boot, "  core-local memory offset", DVAR(apicID),
                      (void*)(KernelCLM::getOffset(apicID)));
  }

  void initThread(size_t apicID) {
    loadKernelSpace();
    gdt.load();
    gdt.tss_kernel_load();

    if (UNLIKELY(this->firstboot)) {
      KernelCLM::initOffset(apicID);
      cpu::initHWThreadID(apicID);
      cpu::initSyscallEntry(stacks[apicID]);
      idt.load();
      async::initLocalPlace(apicID);
      mythos::lapic.init();
      localScheduler.set(&getScheduler(apicID));
      getLocalScheduler().init(&async::places[apicID]);
      Plugin::initPluginsOnThread(apicID);
      this->firstboot = false;
    } else {
      cpu::initSyscallEntry();
      idt.load();
      // not needed: mythos::lapic.init();
    }
  }

  TSS64 tss_kernel;
  GdtAmd64 gdt;
  static IdtAmd64 idt;
  bool firstboot;
};

  } // namespace boot
} // namespace mythos
