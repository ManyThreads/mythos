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
 * Copyright 2016 Randolf Rotta, Robert Kuban, Maik Krüger, and contributors, BTU Cottbus-Senftenberg
 */
#pragma once

#include "cpu/CoreLocal.hh"
#include "util/compiler.hh"
#include <cstdint> // for uint32_t etc

namespace mythos {
  namespace cpu {

    /** Stores a threads execution state for later comeback.
     *
     * Not aligned per default, because even the syscall entry has to
     * touch two cache lines anyway.
     *
     * @warning Do not change the layout without thinking. It is used by
     * the system call and irq entry end exit assembler codes.
     * See thread-state-layout.h.
     */
    struct ThreadState
    {
      uint64_t rip; //< from %rcx, written by the syscall entry point
      uint64_t rflags; //< from %r11, written by the syscall entry point
      uint64_t rsp; //< from %rsp, written by the syscall entry point
      uint64_t rbp, rbx, r12, r13, r14, r15;
      uint64_t fs_base, gs_base;
      uint64_t rax, rcx, rdx, rdi, rsi;
      uint64_t r8, r9, r10, r11;
      // interrupt information
      uint64_t irq, error, cr2;
      uint64_t maySysret; // non-zero if return to user mode can safely use sysret instead of iret
    };

    extern CoreLocal<ThreadState*> thread_state SYMBOL("thread_state") KERNEL_CLM_HOT;
    extern CoreLocal<uintptr_t> kernel_stack SYMBOL("kernel_stack") KERNEL_CLM_HOT;

    void initSyscallStack(cpu::ThreadID threadID, uintptr_t stack);
    void initSyscallEntry();

    // forward declaration
    extern void syscall_entry() SYMBOL("syscall_entry");

    /** high-level syscall handler, must be defined by another module, used by syscall_entry.S */
    NORETURN void syscall_entry_cxx(ThreadState*) SYMBOL("syscall_entry_cxx");

    /** return from syscall to user, defined in syscall_entry.S, reads state from thread_state */
    NORETURN void syscall_return(ThreadState*) SYMBOL("syscall_return");

    /** high-level irq handler when entering from user mode, must be
     * defined by another module, used by irq_entry.S */
    NORETURN void irq_entry_user(ThreadState*) SYMBOL("irq_entry_user");

    /** return from interrupt to user, defined in irq_entry.S, reads state from thread_state */
    NORETURN void irq_return_user(ThreadState*) SYMBOL("irq_return_user");

    NORETURN inline void return_to_user() {
      ThreadState* s = thread_state.get();
      if (s->maySysret && s->rip < 0x7FFFFFFFFFFF) syscall_return(s);
      else irq_return_user(s);
    }

    /** Layout of the kernel's execution state on the interrupted kernel's stack.
     *
     * @warning Do not change the layout without thinking. It matches
     * the pushs and pops in irq_entry.S.
     */
    struct KernelIRQFrame
    {
      uint64_t r15, r14, r13, r12, r11, r10, r9, r8, rbp, rdx, rbx, rsi, rdi, rax, rcx;
      uint64_t irq, error, rip, cs, rflags, rsp, ss;
    };

    /** high-level irq handler when interrupted in kernel mode, must be
     * defined by another model, used by irq_entry.S */
    void irq_entry_kernel(KernelIRQFrame*) SYMBOL("irq_entry_kernel");

  } // namespace cpu
} // namespace mythos
