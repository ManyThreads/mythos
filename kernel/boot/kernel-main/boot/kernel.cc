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
/** @file
 * Setup of global memory management and similar; booting all hardware threads
 * to basic environment.
 */

#include <cstdint>
#include "util/compiler.hh"
#include "cpu/GdtAmd64.hh"
#include "cpu/IdtAmd64.hh"
#include "cpu/IrqHandler.hh"
#include "cpu/kernel_entry.hh"
#include "cpu/CoreLocal.hh"
#include "cpu/hwthreadid.hh"
#include "cpu/ctrlregs.hh"
#include "cpu/LAPIC.hh"
#include "cpu/hwthread_pause.hh"
#include "boot/memory-layout.h"
#include "boot/DeployKernelSpace.hh"
#include "boot/cxx-globals.hh"
#include "boot/mlog.hh"
#include "boot/apboot.hh"
#include "boot/kmem.hh"
#include "async/Place.hh"
#include "boot/load_init.hh"
#include "objects/UntypedMemory.hh"
#include "objects/StaticMemoryRegion.hh"
#include "objects/ISchedulable.hh"
#include "objects/SchedulingContext.hh"
#include "boot/memory-root.hh"

ALIGN_4K uint8_t boot_stack[BOOT_STACK_SIZE] SYMBOL("BOOT_STACK");
extern char CLM_ADDR;
extern char CLM_BLOCKEND;
extern char CLM_END;

uint64_t mythos::tscdelay_MHz=2000;

NORETURN void entry_bsp() SYMBOL("entry_bsp");
NORETURN void entry_ap(size_t id) SYMBOL("entry_ap");

void entry_bsp()
{
  mythos::boot::initKernelSpace();
  mythos::boot::mapLapic(mythos::x86::getApicBase()); // make LAPIC accessible
  mythos::GdtAmd64 tempGDT;
  tempGDT.init();
  tempGDT.load();
  mythos::IdtAmd64 tempIDT;
  tempIDT.initEarly();
  tempIDT.load();
  mythos::KernelCLM::init(size_t(&CLM_BLOCKEND-&CLM_ADDR)); // now the core-local variables are working
  mythos::cpu::initHWThreadID(BOOT_MAX_THREADS); // now the own hwThreadID() is working
  mythos::boot::initMLog();
  MLOG_DETAIL(mlog::boot, "CLM", (void*)&CLM_ADDR, (void*)&CLM_BLOCKEND, (void*)&CLM_END);
  MLOG_DETAIL(mlog::boot, "CLM blocksize", (void*)mythos::KernelCLM::getBlockSize());

  mythos::boot::initCxxGlobals(); // init all global variables
  mythos::boot::initMemoryRegions();
  mythos::boot::initKernelMemory(*mythos::boot::kmem_root());
  mythos::boot::apboot(); // does not return, jumps to entry_ap()
  PANIC_MSG(false, "should never reach here");
}

NORETURN void runUser();

void runUser() {
  mythos::async::getLocalPlace().processTasks();
  MLOG_DETAIL(mlog::boot, "trying to execute app");
  mythos::boot::getLocalScheduler().tryRunUser();
  MLOG_DETAIL(mlog::boot, "going to sleep now");
  mythos::cpu::go_sleeping(); // resets the kernel stack!
}

void entry_ap(size_t id)
{
  //asm volatile("xchg %bx,%bx");
  mythos::boot::apboot_thread(id);
  MLOG_DETAIL(mlog::boot, "started hardware thread");

  if (id == mythos::cpu::enumerateHwThreadID(0)) {
    auto res = mythos::boot::load_init(); // start the first application
    OOPS(res);
  }

  runUser();
}

void mythos::cpu::sleeping_failed()
{
  mythos::async::getLocalPlace().enterKernel();
  MLOG_DETAIL(mlog::boot, "sleeping failed without visible interrupt");
  runUser();
}

void mythos::cpu::syscall_entry_cxx(mythos::cpu::ThreadState* ctx)
{
  mythos::async::getLocalPlace().enterKernel();
  MLOG_DETAIL(mlog::boot, "user system call", DVARhex(ctx->rdi), DVARhex(ctx->rsi),
      DVARhex(ctx->rip), DVARhex(ctx->rsp));
  mythos::handle_syscall(ctx);
  runUser();
}

void mythos::cpu::irq_entry_user(mythos::cpu::ThreadState* ctx)
{
  mythos::async::getLocalPlace().enterKernel();
  MLOG_DETAIL(mlog::boot, "user interrupt", DVARhex(ctx->irq), DVARhex(ctx->error),
      DVARhex(ctx->rip), DVARhex(ctx->rsp));
  if (ctx->irq<32) {
    mythos::handle_trap(ctx); // handle traps, exceptions, bugs from user mode
  } else {
    // TODO then external and wakeup interrupts
    mythos::lapic.endOfInterrupt();
  }
  runUser();
}

void mythos::cpu::irq_entry_kernel(mythos::cpu::KernelIRQFrame* ctx)
{
  MLOG_DETAIL(mlog::boot, "kernel interrupt", DVARhex(ctx->irq), DVARhex(ctx->error),
      DVARhex(ctx->rip), DVARhex(ctx->rsp));
  bool wasbug = handle_bugirqs(ctx);
  bool nested = mythos::async::getLocalPlace().enterKernel();
  // initiate irq processing: first kernel bugs
  if (!wasbug) {
    // TODO then external and wakeup interrupts
    MLOG_INFO(mlog::boot, "ack the interrupt");
    mythos::lapic.endOfInterrupt();
  }

  if (!nested) runUser();
  // else simply return and let the interrupted kernel continue
}
