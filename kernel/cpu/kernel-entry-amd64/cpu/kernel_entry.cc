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
#include "cpu/kernel_entry.hh"
#include "cpu/gdt-layout.h"
#include "cpu/gdt-layout.h"
#include "cpu/ctrlregs.hh"

namespace mythos {
  namespace cpu {

    CoreLocal<ThreadState*> thread_state;
    CoreLocal<uintptr_t> kernel_stack;

    void initSyscallEntry(uintptr_t stack) {
      kernel_stack.set(stack);
      thread_state.set(nullptr);
      initSyscallEntry();
    }

    void initSyscallEntry() {
      x86::setStar(SEGMENT_KERNEL_CS, SEGMENT_USER_CS32+3);
      x86::setLStar(&syscall_entry);
      x86::setFMask(x86::FLAG_TF | x86::FLAG_DF | x86::FLAG_IF | x86::FLAG_IOPL);
      //x86::setMSR(x86::MSR_EFER, x86::getMSR(x86::MSR_EFER) | x86::EFER_SCE); // done in start.S
    }

  } // namespace cpu
} // namespace mythos
