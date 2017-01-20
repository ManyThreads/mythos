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
#include "boot/memory-layout.h"

ENTRY(_start_bsp)

/** ELF programm header for the loader.
 *
 * See ftp://ftp.gnu.org/old-gnu/Manuals/ld-2.9.1/html_node/ld_23.html
 * and the ELF ABI defines how this shall be interpreted.
 *
 * Two sections init and kern are defined with type PT_LOAD.
 * PT_LOAD indicates that this segment should be loaded from the file.
 * The sections below are assigned to segments by ":name" at the
 * end of the section description.
 */
PHDRS
{
    init PT_LOAD;
    kern PT_LOAD;
    data PT_LOAD;
}

SECTIONS
{
    /* LOAD_ADDR is begin of the init code and data */
    /* load the boot code at VMA=LMA LOAD_ADDR */
    .init LOAD_ADDR : AT(LOAD_ADDR)
    {
        KEEP(*(.init))
    } : init /* put into init segment */
    PROVIDE (LOAD_END = .);

    /* the begin of the upper half code and data in physical memory */
    PROVIDE (KERN_BEGIN = .);

    /* All following sections will be placed into the upper half
    kernel address range. Thus, the Virtual Memory Address VMA will be
    large. But the Load Memory Address LMA has to be in physical addresses still. */
    .text (KERN_BEGIN + VIRT_ADDR) : AT (KERN_BEGIN) {
        *(.text.hot .text.hot.*)
        *(".text" ".text$" .text .text.* .gnu.linkonce.t.*)
    } : kern

    .rodata : { *(.rodata .rodata.* .gnu.linkonce.r.*) }
    .eh_frame : { *(.eh_frame) }
    .fixup : { FIXUP_START = .; *(.fixup); FIXUP_END = .; }
    . = ALIGN(2M); /* page aligned */
    PROVIDE (KERN_ROEND = . - VIRT_ADDR);

    .init_array : {
      CTORS_START = .;
      KEEP(*(SORT(.init_array.*)))
      KEEP(*(.init_array))
      KEEP(*(SORT(.ctors.*)))
      KEEP(*(.ctors))
      CTORS_END = .;
    } : data

    .data : {
      *(.data .data.* .gnu.linkonce.d.*)
      . = ALIGN(64); /* cache line aligned */
      CLM_ADDR = ABSOLUTE(.);
      SORT_BY_ALIGNMENT (*)(.kernel_clm.hot)
      SORT_BY_ALIGNMENT (*)(.kernel_clm)
      . = ALIGN(64); /* cache line aligned */
      CLM_BLOCKEND = ABSOLUTE(.);
    } : data

    .bss : {
      . += BOOT_MAX_THREADS*(CLM_BLOCKEND - CLM_ADDR);
      CLM_END = ABSOLUTE(.);
      *(.bss .bss.* .gnu.linkonce.b.*)
    }

    . = ALIGN(2M); /* page aligned */
    PROVIDE (KERN_END = . - VIRT_ADDR); /* the physical end of the upper half stuff */

    /DISCARD/ :
    {
        *(.note.GNU-stack)
        *(.comment*)
    }
}
