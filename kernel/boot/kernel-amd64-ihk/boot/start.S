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
 * Copyright 2016 Randolf Rotta, Robert Kuban, and contributors, BTU Cottbus-Senftenberg
 */
#include "boot/memory-layout.h"

.extern ap_startup_stacks
.extern BOOT_PML4
.extern BOOT_STACK
.extern entry_bsp
.extern entry_ap

// EFER bit 0: SYSCALL instruction enable
// EFER bit 8: IA-32e mode enable
// EFER bit 11: enable noexecute bit in page tables
#define EFER_MASK    0xFFFFFFFF
#define EFER_ENABLE  0x00000901
#define EFER_MSR     0xc0000080

// CR0 bit 0: PE protected mode enable
// CR0 bit 1: MP monitor co-processor
// CR0 bit 4: ET 80387 coprocessor
// CR0 bit 5: NE enable x87 floating point error reporting
// CR0 bit 16: WP enable write protection
// CR0 bit 18: AM enable alignment checks (needs EFLAGS for ring3)
// CR0 bit 31: PG enable paging
#define CR0_MASK     0xFFFFFFFF
#define CR0_ENABLE   0x80050033

// CR4 bit 5: enable PAE mode
// -CR4 bit 8: PCE Performance-Monitoring Counter enable for user mode
// -CR4 bit 6: MCE enable Machine Check Exception
// -CR4 bit 9: OSFXSR Operating system support for FXSAVE and FXRSTOR instructions
// -CR4 bit 10: OSXMMEXCPT Operating System Support for Unmasked SIMD Floating-Point Exceptions
// -CR4 bit 13: VMXE enable Virtual Machine Extensions
// -CR4 bit 16: FSGSBASE Enables the instructions
// -CR4 bit 17: PCIDE enable TLB PCIDs for faster address space switching
// -CR4 bit 18: OSXSAVE XSAVE and Processor Extended States Enable
// -CR4 bit 20: SMEP Supervisor Mode Execution Protection Enable
// -CR4 bit 21: SMAP Supervisor Mode Access Prevention Enable
#define CR4_MASK     0xFFFFFFFF
#define CR4_ENABLE   0x00000020

.section .rodata

.balign  8
//.global _boot_gdt_start
_boot_gdt_start:
    .quad   0x0000000000000000      // null descriptor
    .quad   0x0000000000000000      // 0x08 unused
    .quad   0x00a09b000000ffff      // 0x10 kernel code
    .quad   0x00a093000000ffff      // 0x18 kernel data
_boot_gdt_end:
    .word   0                       // padding to align ...
_boot_gdt:
    .word   _boot_gdt_end - _boot_gdt_start - 1     // limit: length of the GDT
    .quad   _boot_gdt_start                 // base


.section .text

/** \f _start_bsp64_pregdt is called from ihk-entry.cc */
.code64
.balign 8
.global _start_bsp64_pregdt
.type _start_bsp64_pregdt, @function
_start_bsp64_pregdt:
    mov $(BOOT_STACK+BOOT_STACK_SIZE), %rsp    // set up boot kernel stack
    // load our own segment descriptor table, see above in this file
    // the segments 0x10 and 0x18 match the final GDT of the kernel
    lgdt    _boot_gdt
    // switch code segment to 0x10 instead of leftovers from the loader
    // create a stack frame like from a far call and return
    push    $0x10 // KERNEL_CS
    push    $_start_bsp64
    retfq

_start_bsp64:
    mov     $0x18, %cx      // kernel data selector
    mov     %cx, %ss
    mov     %cx, %ds
    mov     %cx, %es
    mov     %cx, %fs
    mov     %cx, %gs
    cld // for gcc code

    mov     $(EFER_MSR), %ecx       // select the EFER MSR
    rdmsr                           // read EFER
    and     $(EFER_MASK), %eax
    or      $(EFER_ENABLE), %eax
    wrmsr                           // write EFER

    // go to the stage 3 boot code, and never come back
    xor %rbp, %rbp
    pushq   $0 // fake return address as first stack frame
    jmp entry_bsp



.code16
.balign 8
.global _dbg_trampoline
base = .
_dbg_trampoline:

.balign 8
jmp _dbg_trampoline_code

.org 8
_dbg_table:
  .quad 0x0

_dbg_pml4:
  .quad 0x0

_dbg_startap:
  .quad 0x0

dbg_ljmp32: // overwritten in (II)
  .long dbg_32prot - base
  .word 0x20

boot_gdtptr:
  .short  boot_gdt32_end - boot_gdt32
   // overwritten in (I)
  .long boot_gdt32 - base

dbg_ljmp64:
   .long dbg_64 - base
   .word 0x10, 0 

_dbg_trampoline_code:
  cli
  wbinvd
  movw  %cs, %ax
  movw  %ax, %ds
  movw  %ax, %es
  movw  %ax, %ss

  // calculate address of trampoline in ebx
  xorl  %ebx, %ebx
  movw  %cs, %bx
  shll  $4, %ebx

  // update gdt address (I)
  //mov $(boot_gdt32 - base), %eax
  //addl %ebx, %eax
  //mov %eax, boot_gdtptr + 2 - base
  addl %ebx, boot_gdtptr + 2 - base

  // update longjump into 32bit (II)
  //mov $(dbg_32prot - base), %eax
  //addl %ebx, %eax
  //mov %eax, dbg_ljmp32 - base
  addl %ebx, dbg_ljmp32 - base

  // update longjump into 64bit
  addl %ebx,  dbg_ljmp64 - base

  mfence
  movl $1, _dbg_trampoline-base
  mfence

  lgdtl boot_gdtptr-base
  lidtl boot_idtptr-base

  jmp test
test:

  movl  %cr0, %edx
  orb $1, %dl
  movl  %edx, %cr0

  mfence
  movl $2, _dbg_trampoline-base
  mfence


  ljmpl *(dbg_ljmp32 - base)

.balign  4
boot_idtptr:
  .short  0
  .long 0


  .balign  4
boot_gdt32:
  .quad 0
  .quad 0
  .quad 0x00af9b000000ffff
  .quad 0x00cf93000000ffff
  .quad 0x00cf9b000000ffff
  .quad 0x0000890000000067
boot_gdt32_end:

.balign 8
.code32
dbg_32prot:

  movl  $0x18, %eax
  movl  %eax, %ds
  movl  %eax, %ss

  mfence
  movl $3, (%ebx)
  mfence

  // setup paging
  mov %cr4, %edx
  and     $(CR4_MASK), %edx
  or      $(CR4_ENABLE), %edx
  mov %edx, %cr4

  movl (_dbg_table - base)(%ebx), %eax // !!!!  use variable
  movl %eax, %cr3 // load phys pointer of the PML4 table

  // setup EFER
  mov $(EFER_MSR), %ecx
  xor %eax, %eax
  rdmsr
  and     $(EFER_MASK), %eax
  or      $(EFER_ENABLE), %eax
  wrmsr

  mfence
  movl $4, (%ebx)
  mfence

  mov %cr0, %edx
  and     $(CR0_MASK), %edx
  or      $(CR0_ENABLE), %edx
  mov %edx, %cr0

  ljmp *(dbg_ljmp64 - base)(%ebx)

.balign 8
.code64
dbg_64:

  mfence
  movl $5, (%ebx)
  mfence

  mov (_dbg_pml4 - base)(%rbx), %rax // !!!!  use variable
  mov %rax, %cr3 // load phys pointer of the PML4 table

  mfence
  movl $6, (%ebx)
  mfence

  movq	(_dbg_startap - base)(%rbx),%r8
  jmp *%r8

dbg_loop:
  hlt
  jmp dbg_loop


.global _dbg_trampoline_end
_dbg_trampoline_end:


.code64
.balign 8

.global _start_ap64_pregdt
.type _start_ap64_pregdt, @function
_start_ap64_pregdt:

  mfence
  movl $7, (%ebx)
  mfence

        // get initial LAPIC ID from bits(cpuid(1).ebx,31,24)
        mov $1, %rax
        mov $0, %rcx
        cpuid
        shr $24, %rbx
        and $0xFF, %rbx

        // load stack address
        movq ap_startup_stacks(,%rbx,8), %rsp

        // load our own segment descriptor table, see above in this file
        //lgdt    _boot_gdt
        // switch code segment to 0x10 instead of leftovers from the loader
        // create a stack frame like from a far call and return
        push    $0x10 // KERNEL_CS
        push    $_start_ap64
        retfq

.global _start_ap64 // needed?
.type _start_ap64, @function // needed?
_start_ap64:
        //xchg %bx,%bx // bochs magic breakpoint
        mov $0x18, %ecx // kernel data selector
        mov %ecx, %ss
        mov %ecx, %ds
        mov %ecx, %es
        mov %ecx, %fs
        mov %ecx, %gs
        cld // for gcc code

        // go to the stage 3 boot code, and never come back
        xor %rbp, %rbp
        movq %rdi, %rsi // reason as 2nd argument (rsi)
        movq %rbx, %rdi // hwt address as first argument (rdi) for main_ap
        pushq   $0 // fake return address as first stack frame
        jmp entry_ap
