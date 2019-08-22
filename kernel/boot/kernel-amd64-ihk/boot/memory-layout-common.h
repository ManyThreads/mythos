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
/** @file memory-layout-common.h
 * Memory layout definitions during booting. These definitions
 * connect the linker script with the startup assembler code and the
 * initial kernel C/C++ code.
 *
 * See doc/memory-layout.md for a discussion of these constants. Never
 * ever change something here without updating the documentation!
 *
 * This file is included by the linker script and therefore can
 * contain only preprocessor macros and definition.
 *
 * To ensure proper dependency injection, this file shall be included
 * only by boot.ld, start.s, and kernel.cc.
 */
#pragma once

/** where the code and data of the the kernel will be places in the
 * logical address space (load address is physical, virtual address is
 * logical).
 *
 * \warning The kernel image has to be placed in the upmost 2GiB of
 * the logical address space in order to be able to use negative 32bit
 * constants for absolute addressing. Otherwise the kernel code
 * becomes much less efficient.
 */
#define VIRT_ADDR           0xFFFFFFFFFE801000
#define LOAD_ADDR           0xFFFFFFFFFE801000 /* should not be used. needed by kmem-common */

/** position and size of the kernels direct mapped area,
 * which will contain all kernel objects
 */
#define KERNELMEM_ADDR      0xffff810000001000
#define KERNELMEM_SIZE      0x0000000100000000 /* 4GB = 32bit */

#define DEVICES_ADDR        0xffff810100000000
/** LAPIC_START fixed mapping of the hardware thread's local APIC */
#define LAPIC_ADDR          0xffff810100001000
/** IOAPIC_START fixed mapping of the global ioapics */
#define IOAPIC_ADDR			0xffff810100002000
#define KERNELSTACKS_ADDR   0xffff810100200000

#define BOOT_STACK_SIZE     2*4096
#define CORE_STACK_SIZE     2*4096
#define NMI_STACK_SIZE      512

#define MYTHOS_MAX_THREADS    244
#define MYTHOS_MAX_APICID     256
