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
/*_mboot_header:
        .long MULTIBOOT_HEADER_MAGIC
        .long MULTIBOOT_HEADER_FLAGS
        .long -(MULTIBOOT_HEADER_MAGIC+MULTIBOOT_HEADER_FLAGS)*/

/** Global Descriptor Table (GDT) during boot.
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
_boot_gdt_end:
        .word   0                       // padding to align ...
_boot_gdt:
        .word   _boot_gdt_end - _boot_gdt_start - 1     // limit: length of the GDT
        .quad   _boot_gdt_start                 // base

/*.align 8
.global _mboot_table
_mboot_table:   .quad   0
.global _mboot_magic
_mboot_magic:   .quad   0*/

/* just for testing */
//.global _host_info_ptr
//_host_info_ptr:   .quad   0

/* start code for the bootstrap processor BSP, i.e. the first core */
/*.code32
.align  8
.global _start_bsp
.type _start_bsp, @function
_start_bsp:
        // eax and ebx contains the multiboot magic and table pointer,
        // lets store this for later.
        mov %eax, _mboot_magic
        mov %ebx, _mboot_table

        // activate the initial page table
        mov     $(EFER_MSR), %ecx       // select the EFER MSR
        rdmsr                           // read EFER
        and     $(EFER_MASK), %eax
        or      $(EFER_ENABLE), %eax
        wrmsr                           // write EFER
        mov     %cr4, %edx
        and     $(CR4_MASK), %edx
        or      $(CR4_ENABLE), %edx
        mov     %edx, %cr4
        mov     $(BOOT_PML4-VIRT_ADDR), %edx // pointer to the Page Directory
        mov     %edx, %cr3
        mov     %cr0, %edx
        and     $(CR0_MASK), %edx
        or      $(CR0_ENABLE), %edx
        mov     %edx, %cr0  // switch to protected mode, with paging enabled and write protect

        // load our own segment descriptor table
        lgdt    _boot_gdt
        ljmp    $0x10, $_start_bsp64    // switch CS to 64bit mode
*/
.code64
.align 8
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

// ---------------

/*.code16
.align  16
.global _setup_ap
_setup_ap:
        cli
        // init paging and load stage 1 page table
        mov $(EFER_MSR), %ecx // select the EFER MSR
        rdmsr
        and     $(EFER_MASK), %eax
        or      $(EFER_ENABLE), %eax
        wrmsr
        mov %cr4, %edx
        and     $(CR4_MASK), %edx
        or      $(CR4_ENABLE), %edx
        mov %edx, %cr4
        mov $BOOT_PML4-VIRT_ADDR, %edx
        mov %edx, %cr3 // load phys pointer of the PML4 table
        mov %cr0, %edx
        and     $(CR0_MASK), %edx
        or      $(CR0_ENABLE), %edx
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
.
        mov     $(EFER_MSR), %ecx       // select the EFER MSR
        rdmsr         global _setup_ap_end
_setup_ap_end:

.code64
.align  8
_start_ap64_low:
        movq $0, %rdi
        jmp _start_ap64
*/
.section .text

.code16
.align 8
.global _dbg_trampoline
base = .
_dbg_trampoline:

.align 8
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

.align  4
boot_idtptr:
  .short  0
  .long 0


  .align  4
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

  // load gdt via relative address
  //lgdtl %cs:__gdt_desc - _setup_ap

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

/*
        // load gdt via relative address
        lgdtl %cs:__gdt_desc - _setup_ap

        // jump to next stage
        // this switches from 16 to 64 bit mode and actives the paging
        ljmpl $0x10, $_start_ap64_low

        // need a local gdt descriptor, because lgdtl can only handle 16bit relative addresses

.align  8
_dbg_gdt_start:
        .quad   0x0000000000000000      // null descriptor
        .quad   0x0000000000000000      // 0x08 unused
        .quad   0x00a09b000000ffff      // 0x10 kernel code
        .quad   0x00a093000000ffff      // 0x18 kernel data
_dbg_gdt_end:
        .word   0                       // padding to align ...
_dbg_gdt:
        .word   _boot_gdt_end - _boot_gdt_start - 1     // limit: length of the GDT
        .quad   _boot_gdt_start                 // base
*/

.global _dbg_trampoline_end
_dbg_trampoline_end:


.code64
.align 8

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
