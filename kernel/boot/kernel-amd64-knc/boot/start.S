/* -*- mode:asm; indent-tabs-mode:nil -*- */
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
 * Copyright 2016 Randolf Rotta, Robert Kuban, and contributors, BTU Cottbus-Senftenberg
 */
#include "boot/memory-layout.h"

.extern ap_startup_stacks
.extern BOOT_PML4
.extern BOOT_STACK
.extern entry_bsp
.extern entry_ap

.section .init, "awx"  // the following code and data is in the low memory init section

/** Contains physical address of a struct
 * with pointers to communication channels.
 * It will be read by the host tools.
 */
.align  8
.global _host_info_ptr
_host_info_ptr:
        .quad   0
.global _host_debug_info
_host_debug_info:
        .quad   0

/** Global Descriptor Table (GDT) during boot wiht five segments.
 * see 3.4.5.1 Code- and Data-Segment Descriptor Types in Intel 253668-048US.
 * granularity 0xa0 = 10100000b => limit granularity 4kb, operand size unset, 64bit mode
 * access 0x9b = 10011011b => present, ring0, code/data, Execute/Read accessed
 *  0011 read/write accessed
 *  1001 execute only accessed
 *  1011 execute/read accessed
 *  access 0xfb = 11111011 => present, ring3, code/data, Execute/Read accessed
 *  limit can be 0 because it is not used in 64bit mode
*/
.align  8
.global _boot_gdt_start
_boot_gdt_start:
        .quad   0x0000000000000000      // null descriptor
        .quad   0x0000000000000000      // 0x08 unused
        .quad   0x00a09b000000ffff      // 0x10 kernel code
        .quad   0x00a093000000ffff      // 0x18 kernel data
        .quad   0x00a09b000000ffff      // 0x20 user code in ring 3
        .quad   0x00a093000000ffff      // 0x28 user data in ring 3
_boot_gdt_end:
        .word   0                       // padding to align ...
_boot_gdt:
        .word   _boot_gdt_end - _boot_gdt_start - 1     // limit: length of the GDT
        .quad   _boot_gdt_start                 // base

/* start code for the bootstrap processor BSP, i.e. the first core */
.code64
.align  8
.global _start_bsp
.type _start_bsp, @function
_start_bsp:
        // movq $__LINE__, _host_debug_info
        // activate the initial page table
        mov     $0xc0000080, %rcx       // select the EFER MSR
        rdmsr                           // read EFER
        or      $0x101, %rax            // set LME (bit8) and enable SYSCALL (bit0), XXX enable noexecute (bit11)
        wrmsr                           // write EFER
        mov     %cr4, %rdx
        or      $0x20, %edx             // enable PAE mode (bit5), XXX enable SMAP,SMEP,PCE later?
        mov     %rdx, %cr4
        mov     $(BOOT_PML4-VIRT_ADDR), %rdx             // pointer to the Page Directory
        mov     %rdx, %cr3
        mov     %cr0, %rdx
        or     $0x80010001, %edx        // enable PE (bit0), WP (bit16), PG (bit31)
        mov     %rdx, %cr0              // switch to protected mode, with paging enabled and write protect
        // load our own segment descriptor table
        lgdt    _boot_gdt
        // switch code segment to 0x10 instead of whatever was left from fboot1
        mov $(BOOT_STACK+BOOT_STACK_SIZE), %rsp    // set up boot kernel stack
        push    $0x10 // KERNEL_CS
        push    $_start_bsp64
        retfq

.code64
_start_bsp64:
        mov     $0x18, %cx      // kernel data selector
        mov     %cx, %ss
        mov     %cx, %ds
        mov     %cx, %es
        mov     %cx, %fs
        mov     %cx, %gs
        cld // for gcc code

        // go to the stage 3 boot code, and never come back
        xor %rbp, %rbp
        pushq   $0 // fake return address
        jmp entry_bsp

// ---------------

.code16
.align  16
.global _setup_ap
_setup_ap:
        cli
        // init paging and load stage 1 page table
        mov $0xc0000080, %ecx // select the EFER MSR
        rdmsr
        or $0x101, %eax // set LME (bit8) and enable SYSCALL (bit0), XXX enable noexecute (bit11)
        wrmsr
        mov %cr4, %edx
        or $0x20, %edx // enable PAE mode (bit5), XXX enable SMAP,SMEP,PCE later?
        mov %edx, %cr4
        mov $(BOOT_PML4-VIRT_ADDR), %edx
        mov %edx, %cr3 // load phys pointer of the PML4 table
        mov %cr0, %edx
        or $0x80010001, %edx // enable PE (bit0), WP (bit16), PG (bit31)
        mov %edx, %cr0

        // load gdt via relative address
        lgdtl %cs:__gdt_desc - _setup_ap

        // jump to next stage
        // this switches from 16 to 64 bit mode and actives the paging
        ljmpl $0x10, $_start_ap64_low

        // need a local gdt descriptor, because lgdtl can only handle 16bit relative addresses
__gdt_desc:
        .word _boot_gdt_end - _boot_gdt_start - 1
        .quad _boot_gdt_start
.global _setup_ap_end
_setup_ap_end:

.code64
.align  8
_start_ap64_low:
        jmp _start_ap64

.section .text
.code64
.align  8
.global _start_ap64
.type _start_ap64, @function
_start_ap64:
        //xchg %bx,%bx // bochs magic breakpoint
        mov $0x18, %ecx // kernel data selector
        mov %ecx, %ss
        mov %ecx, %ds
        mov %ecx, %es
        mov %ecx, %fs
        mov %ecx, %gs
        cld // for gcc code

        // get initial LAPIC ID from bits(cpuid(1).ebx,31,24)
        mov $1, %rax
        mov $0, %rcx
        cpuid
        shr $24, %rbx
        and $0xFF, %rbx

        // load stack address and hwt address as first argument for main_ap
        movq ap_startup_stacks(,%rbx,8), %rsp
        movq %rbx, %rdi

        // go to the stage 3 boot code, and never come back
        xor %rbp, %rbp
        pushq   $0 // fake return address
        jmp entry_ap
