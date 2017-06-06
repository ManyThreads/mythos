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

#include "cpu/IrqHandler.hh"
#include "cpu/kernel_entry.hh"
#include "cpu/ctrlregs.hh"
#include "cpu/stacktrace.hh"
#include "boot/mlog.hh"
#include "cpu/hwthread_pause.hh"

namespace mythos {

  class IrqKernelFault
    : public IIrqHandler<cpu::KernelIRQFrame*>
  {
  public:
    void process(cpu::KernelIRQFrame* ctx) {
      MLOG_ERROR(mlog::boot, "kernel fault", DVAR(ctx->irq), DVAR(ctx->error),
		       DVARhex(x86::getCR2()));
      MLOG_ERROR(mlog::boot, "...", DVARhex(ctx->rip), DVAR(ctx->cs), DVARhex(ctx->rflags),
		       DVARhex(ctx->rsp), DVAR(ctx->ss));
      MLOG_ERROR(mlog::boot, "...", DVARhex(ctx->rax), DVARhex(ctx->rbx), DVARhex(ctx->rcx),
		       DVARhex(ctx->rdx), DVARhex(ctx->rbp), DVARhex(ctx->rdi),
		       DVARhex(ctx->rsi));
      MLOG_ERROR(mlog::boot, "...", DVARhex(ctx->r8), DVARhex(ctx->r9), DVARhex(ctx->r10),
		       DVARhex(ctx->r11), DVARhex(ctx->r12), DVARhex(ctx->r13),
		       DVARhex(ctx->r14), DVARhex(ctx->r15));
      MLOG_ERROR(mlog::boot, "stacktrace:");
      for (auto& frame : StackTrace(ctx->rbp)) {
	MLOG_ERROR(mlog::boot, '\t', frame.ret);
      }
      // TODO information from interesting kernel data structures
      mythos::sleep_infinitely();
    }
  };

  class IrqKernelNop
    : public IIrqHandler<cpu::KernelIRQFrame*>
  {
  public:
    void process(cpu::KernelIRQFrame* ctx) {
      MLOG_INFO(mlog::boot, "kernel irq", DVAR(ctx->irq), DVAR(ctx->error),
		      DVARhex(ctx->rip), DVARhex(ctx->rsp));
    }
  };
  
  IrqKernelFault irq_kfault;
  IrqKernelNop irq_knop;
  
  IIrqHandler<cpu::KernelIRQFrame*>* irq_kernelbugs[32] = {
    &irq_kfault, &irq_kfault, // 0,1
    &irq_knop, &irq_knop, &irq_knop, // 2,3,4
    &irq_kfault, &irq_kfault, &irq_kfault, &irq_kfault, &irq_kfault, // 5,6,7,8,9
    &irq_kfault, &irq_kfault, &irq_kfault, &irq_kfault, &irq_kfault, // 10,11,12,13,14
    &irq_kfault, &irq_kfault, &irq_kfault, &irq_kfault, &irq_kfault, // 15,16,17,18,19
    &irq_kfault, &irq_knop, &irq_knop, &irq_knop, &irq_knop, // 20,21,22,23,24
    &irq_knop, &irq_knop, &irq_knop, &irq_knop, &irq_knop, // 25,26,27,28,29,30
    &irq_knop // 31
  };

  bool handle_bugirqs(cpu::KernelIRQFrame* ctx)
  {
    if (ctx->irq>=32) return false;
    irq_kernelbugs[ctx->irq]->process(ctx);
    return true;
  }
  
} // namespace mythos
