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

#define IA32_FS_BASE 0xc0000100
#define IA32_GS_BASE 0xc0000101
#define IA32_KERNEL_GS_BASE 0xc0000102

#define TS_RIP      0x0000 // first cache line
#define TS_RFLAGS   0x0008
#define TS_RSP      0x0010
#define TS_RBP      0x0018
#define TS_RBX      0x0020
#define TS_R12      0x0028
#define TS_R13      0x0030
#define TS_R14      0x0038 
#define TS_R15      0x0040 // second cache line
#define TS_FS_BASE  0x0048
#define TS_GS_BASE  0x0050
#define TS_RAX      0x0058
#define TS_RCX      0x0060
#define TS_RDX      0x0068
#define TS_RDI      0x0070
#define TS_RSI      0x0078 
#define TS_R8       0x0080 // third cache line
#define TS_R9       0x0088
#define TS_R10      0x0090
#define TS_R11      0x0098
#define TS_IRQ      0x00a0
#define TS_ERROR    0x00a8
#define TS_CR2      0x00b0
#define TS_MAYSYSRET 0x00b8

#define IRQSTACK_RAX    0x00
#define IRQSTACK_RCX    0x08
#define IRQSTACK_IRQNO  0x10
#define IRQSTACK_ERROR  0x18
#define IRQSTACK_RIP    0x20
#define IRQSTACK_CS     0x28
#define IRQSTACK_RFLAGS 0x30
#define IRQSTACK_RSP    0x38
#define IRQSTACK_SS     0x40
