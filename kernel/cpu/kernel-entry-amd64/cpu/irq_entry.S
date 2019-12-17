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

// assumes that interrupts are already disabled (interrupt-gate instead of trap-gate) !

.extern thread_state
.extern kernel_stack
.extern irq_entry_user
.extern irq_entry_kernel

// macro for interrupt entry without error code
.macro INTERRUPT_ENTRY no
        interrupt_entry_\no:
        push    $0                      // error-code (no error here)
        push    $\no                    // the interrupt-number
        jmp     interrupt_common
.endm

// macro for interrupt entry with error code already on the stack
.macro INTERRUPT_ENTRY_ERROR no
        interrupt_entry_\no:
        push    $\no                    // the interrupt-number
        jmp     interrupt_common
.endm

// the interrupt entry routines
.text
.align  64
INTERRUPT_ENTRY 0
INTERRUPT_ENTRY 1
INTERRUPT_ENTRY 2
INTERRUPT_ENTRY 3
INTERRUPT_ENTRY 4
INTERRUPT_ENTRY 5
INTERRUPT_ENTRY 6
INTERRUPT_ENTRY 7
INTERRUPT_ENTRY_ERROR 8
INTERRUPT_ENTRY 9
INTERRUPT_ENTRY_ERROR 10
INTERRUPT_ENTRY_ERROR 11
INTERRUPT_ENTRY_ERROR 12
INTERRUPT_ENTRY_ERROR 13
INTERRUPT_ENTRY_ERROR 14
INTERRUPT_ENTRY 15
INTERRUPT_ENTRY 16
INTERRUPT_ENTRY_ERROR 17

.irp vector,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255
        INTERRUPT_ENTRY \vector
.endr

// a helper table with the addresses of these routines
.section .rodata
.align  8
.global interrupt_entry_table
interrupt_entry_table:
.irp vector,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255
        .quad interrupt_entry_\vector
.endr


.text
.align 16
interrupt_common:
        xchg %bx, %bx
        // interrupt frame, error code and number already on the stack
        push %rcx   // save regs to get register for the following fun
        push %rax   // save regs to get register for the following fun

        // test cs==USER_CS to see if we came from user mode
        mov IRQSTACK_CS(%rsp), %cx
        cmp $(SEGMENT_USER_CS+3), %cx
        jne interrupt_kernel  // jmp if kernel mode was interrupted

        // we got an interrupt from user mode => safe everything in the current thread_state
        cld  // safe standard value for the direction flag
        swapgs          // swap the current GS base with the kernel's GS base from MSR
        mov %gs:thread_state, %rax // get state object ptr from core-local variable
        mov IRQSTACK_ERROR(%rsp), %rcx
        mov %rcx, TS_ERROR(%rax)
        mov IRQSTACK_IRQNO(%rsp), %rcx
        mov %rcx, TS_IRQ(%rax)
        mov %cr2, %rcx
        mov %rcx, TS_CR2(%rax)
        mov IRQSTACK_RAX(%rsp), %rcx
        mov %rcx, TS_RAX(%rax)
        mov IRQSTACK_RCX(%rsp), %rcx
        mov %rcx, TS_RCX(%rax)
        mov IRQSTACK_RIP(%rsp), %rcx
        mov %rcx, TS_RIP(%rax)
        mov IRQSTACK_RFLAGS(%rsp), %rcx
        mov %rcx, TS_RFLAGS(%rax)
        mov IRQSTACK_RSP(%rsp), %rcx
        mov %rcx, TS_RSP(%rax)

        mov %rbp, TS_RBP(%rax)
        mov %rbx, TS_RBX(%rax)
        mov %rdx, TS_RDX(%rax)
        mov %rdi, TS_RDI(%rax)
        mov %rsi, TS_RSI(%rax)
        mov %r8, TS_R8(%rax)
        mov %r9, TS_R9(%rax)
        mov %r10, TS_R10(%rax)
        mov %r11, TS_R11(%rax)
        mov %r12, TS_R12(%rax)
        mov %r13, TS_R13(%rax)
        mov %r14, TS_R14(%rax)
        mov %r15, TS_R15(%rax)
        movq $0, TS_MAYSYSRET(%rax)

        // might be on the NMI stack, but everything is stored safely
        // in the ThreadState, thus we can switch to the bigger kernel
        // stack now
        /// @todo check whether we actually could end up on the NMI stack when coming from user mode!!!
        mov %gs:kernel_stack, %rsp
        mov %rax, %rdi   // 1. argument: pointer to thread_state
        xor %rbp, %rbp
        pushq   $0 // fake return address
        jmp irq_entry_user   // @todo as a call
        // @todo call try to return to user via scheduler
        // @todo call sleep if nothing to return to

interrupt_kernel:
        xchg %bx, %bx
        // put all other registers on the kernel stack
        push %rdi
        push %rsi
        push %rbx
        push %rdx
        push %rbp
        push %r8
        push %r9
        push %r10
        push %r11
        push %r12
        push %r13
        push %r14
        push %r15
        // call the high-level interrupt handler
        // we stay on the NMI stack if it was an NMI, because the kernel's main stack is in use
        mov %rsp, %rdi   // 1. argument: pointer to stack with all information
        call irq_entry_kernel
        // restore registers
        pop %r15
        pop %r14
        pop %r13
        pop %r12
        pop %r11
        pop %r10
        pop %r9
        pop %r8
        pop %rbp
        pop %rdx
        pop %rbx
        pop %rsi
        pop %rdi
        pop %rax
        pop %rcx
        add $(2*8), %rsp // pop irqno and error code
        iretq
