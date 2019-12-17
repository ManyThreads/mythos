/* -*- mode:asm; indent-tabs-mode:nil -*- */
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
#include "cpu/thread-state-layout.h"
#include "cpu/gdt-layout.h"

.extern thread_state
.extern kernel_stack
.extern syscall_entry_cxx

.text
.global syscall_entry
.type syscall_entry, @function
.align 16
syscall_entry:
        // caller-saved: rax, rcx, r11
        // warning: non-maskable interrupt can happen here, needs own stack though!
        swapgs          // swap the current GS base with the kernel's GS base from MSR
        mov %gs:thread_state, %rax // get state object ptr from core-local variable
        mov %rsp, TS_RSP(%rax) // save user's stack
        mov %gs:kernel_stack, %rsp // get kernel's stack
        mov %rcx, TS_RIP(%rax) // instruction pointer
        mov %r11, TS_RFLAGS(%rax) // rflags
        mov %rbp, TS_RBP(%rax)
        //mov %rbx, TS_RBX(%rax)
        //mov %r12, TS_R12(%rax)
        //mov %r13, TS_R13(%rax)
        //mov %r14, TS_R14(%rax)
        //mov %r15, TS_R15(%rax)
        // skip rax, rcx (rax was used on entry, rcx contains user's rip)
        //movq $0, TS_RAX(%rax)
        mov %rdx, TS_RDX(%rax)
        mov %rdi, TS_RDI(%rax)
        mov %rsi, TS_RSI(%rax)
        //mov %r8, TS_R8(%rax)
        //mov %r9, TS_R9(%rax)
        mov %r10, TS_R10(%rax)
        // skip r11 (contains user's rflags)
        movq $1, TS_MAYSYSRET(%rax)
        mov %rax, %rdi   // 1. argument: pointer to thread_state
        xor %rbp, %rbp
        pushq   $0 // fake return address
        jmp syscall_entry_cxx   // @todo as a call
        // @todo call try to return to user via scheduler
        // @todo call sleep if nothing to return to

.global return_to_user_asm
.type return_to_user_asm, @function
// RDI: ThreadState*
return_to_user_asm:
        mov $IA32_FS_BASE, %rcx
        mov TS_FS_BASE(%rdi), %eax
        mov (TS_FS_BASE+4)(%rdi), %edx
        wrmsr   // restore user's FS base
        mov $IA32_KERNEL_GS_BASE, %rcx
        mov TS_GS_BASE(%rdi), %eax
        mov (TS_GS_BASE+4)(%rdi), %edx
        wrmsr   // prepare user's GS base for swapgs

        // check for save and fast sysret, otherwise do iret
        // maysysret is nonzero and rip < 0x7FFFFFFFFFFF
        movabs $0x7ffffffffffe,%rax
        mov TS_RIP(%rdi), %rcx          // instruction pointer
        cmp %rax, %rcx
        setbe %al
        movzbl %al, %eax
        test %rax, TS_MAYSYSRET(%rdi)   // flag for "came from syscall"
        je return_from_irq

        /// @todo reorder thread-state-layout to reflect reduced register passing
        xor %rbx, %rbx
        xor %r12, %r12
        xor %r13, %r13
        xor %r14, %r14
        xor %r15, %r15
        // skip rcx (rcx contains user's rip)
        xor %rax, %rax
        xor %rdx, %rdx
        mov TS_RSI(%rdi), %rsi          // return value: user context
        xor %r8, %r8
        xor %r9, %r9
        xor %r10, %r10
        mov $0x000200, %r11             // don't trust the user's rflags, enforce interrupt enable flag
        mov TS_RBP(%rdi), %rbp          // stack frame base pointer
        mov TS_RSP(%rdi), %rsp          // top of stack
        mov TS_RDI(%rdi), %rdi          // return value: state or error code
        swapgs                          // restore user's GS base
        sysretq

return_from_irq:
        // create fake interrupt frame on the kernel stack
        pushq $(SEGMENT_USER_DS+3)      // SS - user data segment with bottom 2 bits set for ring 3
        pushq TS_RSP(%rdi)              // RSP - user stack from 2nd parameter
        pushq $0x000200                 // RFLAGS - don't trust the user's rflags, enforce interrupt enable flag
        pushq $(SEGMENT_USER_CS+3)      // CS  - user code segment with bottom 2 bits set for ring 3
        pushq %rcx                      // RIP - Instruction pointer after iretq
        // restore all other register
        mov TS_RAX(%rdi), %rax
        mov TS_RBX(%rdi), %rbx
        mov TS_RCX(%rdi), %rcx
        mov TS_RDX(%rdi), %rdx
        mov TS_RSI(%rdi), %rsi
        mov TS_RBP(%rdi), %rbp
        mov TS_R8(%rdi), %r8
        mov TS_R9(%rdi), %r9
        mov TS_R10(%rdi), %r10
        mov TS_R11(%rdi), %r11
        mov TS_R12(%rdi), %r12
        mov TS_R13(%rdi), %r13
        mov TS_R14(%rdi), %r14
        mov TS_R15(%rdi), %r15
        mov TS_RDI(%rdi), %rdi
        swapgs  // restore user's GS base
        iretq   // return to user mode and enable interrupts via rflags IF field
