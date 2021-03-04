/* -*- mode:asm; indent-tabs-mode:nil; -*- */
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

.extern initstack_top
.extern start_c
.extern info_ptr
.extern __libc_init_array
.extern exit

.align 8
.global _start
_start:
        mov initstack_top, %rsp
        xor %rbp, %rbp
        mov %rdi, info_ptr       /* store first argument to global variable */
        call start_c
1:      jmp 1b                  /* should never return from start_c anyway */

/* A handle for __cxa_finalize to manage c++ local destructors.  */
.global __dso_handle
.type __dso_handle,@object
.size __dso_handle,8
.section .bss
.align 8
__dso_handle:
.skip  8
.hidden __dso_handle

/*.section .init
.global _init
.type _init,@function
_init:
        push %rbp
        mov %rsp, %rbp*/

.section .fini
.global _fini
.type _fini,@function
_fini:
        push %rbp
        mov %rsp, %rbp
