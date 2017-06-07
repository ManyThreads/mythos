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

/** magic number for the Multiboot header. */
#define MULTIBOOT_HEADER_MAGIC          0x1BADB002

/** multiboot flags towards the bootloader.
 *
 * https://www.gnu.org/software/grub/manual/multiboot/multiboot.html
 * If bit 0 in the ‘flags’ word is set, then all boot modules loaded along with the operating system must be aligned on page (4KB) boundaries.
 * If bit 1 in the ‘flags’ word is set, then information on available memory via at least the ‘mem_*’ fields of the Multiboot information structure must be included. If the boot loader is capable of passing a memory map (the ‘mmap_*’ fields) and one exists, then it may be included as well.
 * If bit 2 in the ‘flags’ word is set, information about the video mode table must be available to the kernel.
 * If bit 16 in the ‘flags’ word is set, then the fields at offsets 12-28 in the Multiboot header are valid, and the boot loader should use them instead of the fields in the actual executable header to calculate where to load the OS image. This information does not need to be provided if the kernel image is in elf format.
 */
#define MULTIBOOT_HEADER_FLAGS          0x00000000

.align  4
/** multiboot header: just magic, flags and checksum without the optional fields */
_mboot_header:
        .long MULTIBOOT_HEADER_MAGIC
        .long MULTIBOOT_HEADER_FLAGS
        .long -(MULTIBOOT_HEADER_MAGIC+MULTIBOOT_HEADER_FLAGS)

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

.align 8
.global _mboot_table
_mboot_table:   .quad   0
.global _mboot_magic
_mboot_magic:   .quad   0

/* start code for the bootstrap processor BSP, i.e. the first core */
.code64
.align  8
.global _start_bsp
.type _start_bsp, @function
_start_bsp:
        movq $__LINE__, _host_debug_info
        // eax and ebx contains the multiboot magic and table pointer,
        // lets store this for later.
        mov %rax, _mboot_magic
        mov %rbx, _mboot_table

        movq $__LINE__, _host_debug_info
        // activate the initial page table
        mov     $0xc0000080, %rcx       // select the EFER MSR
        rdmsr                           // read EFER
        or      $0x901, %rax            // IA-32e mode enable (bit8), SYSCALL (bit0), noexecute (bit11)
        wrmsr                           // write EFER
        mov     %cr4, %rdx
        or      $0x20, %rdx             // enable PAE mode (bit5), XXX enable SMAP,SMEP,PCE later?
        mov     %rdx, %cr4
        mov     $(BOOT_PML4-VIRT_ADDR), %rdx             // pointer to the Page Directory
        mov     %rdx, %cr3
        mov     %cr0, %rdx
        or     $0x80050033, %edx        // enable PE(bit0), MP(bit1), ET(bit4), NE(bit5), AM(bit18), WP(bit16), PG(bit31)
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
        or $0x901, %eax // IA-32e mode enable (bit8), SYSCALL (bit0), noexecute (bit11)
        wrmsr
        mov %cr4, %edx
        or $0x20, %edx // enable PAE mode (bit5), XXX enable SMAP,SMEP,PCE later?
        mov %edx, %cr4
        mov $(BOOT_PML4-VIRT_ADDR), %edx
        mov %edx, %cr3 // load phys pointer of the PML4 table
        mov %cr0, %edx
        or $0x80050033, %edx // enable PE(bit0), MP(bit1), ET(bit4), NE(bit5), AM(bit18), WP(bit16), PG(bit31)
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
        movq $0, %rdi
        jmp _start_ap64

.code16
.align  16
.global _setup_ap_cc6
_setup_ap_cc6:
        cli
        // init paging and load stage 1 page table
        mov $0xc0000080, %ecx // select the EFER MSR
        rdmsr
        or $0x901, %eax // IA-32e mode enable (bit8), SYSCALL (bit0), noexecute (bit11)
        wrmsr
        mov %cr4, %edx
        or $0x20, %edx // enable PAE mode (bit5), XXX enable SMAP,SMEP,PCE later?
        mov %edx, %cr4
        mov $(BOOT_PML4-VIRT_ADDR), %edx
        mov %edx, %cr3 // load phys pointer of the PML4 table
        mov %cr0, %edx
        or $0x80050033, %edx // enable PE(bit0), MP(bit1), ET(bit4), NE(bit5), AM(bit18), WP(bit16), PG(bit31)
        mov %edx, %cr0

        // load gdt via relative address
        lgdtl %cs:__gdt_desc_cc6 - _setup_ap_cc6

        // jump to next stage
        // this switches from 16 to 64 bit mode and actives the paging
        ljmpl $0x10, $_start_ap64_low_cc6

        // need a local gdt descriptor, because lgdtl can only handle 16bit relative addresses
__gdt_desc_cc6:
        .word _boot_gdt_end - _boot_gdt_start - 1
        .quad _boot_gdt_start
.global _setup_ap_end_cc6
_setup_ap_end_cc6:

.code64
.align  8
_start_ap64_low_cc6:
        movq $1, %rdi
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

        movq ap_startup_stacks(,%rbx,8), %rsp // load stack address
        movq %rdi, %rsi // reason as 2nd argument (rsi)
        movq %rbx, %rdi // hwt address as first argument (rdi) for main_ap

        // go to the stage 3 boot code, and never come back
        xor %rbp, %rbp
        pushq   $0 // fake return address
        jmp entry_ap
