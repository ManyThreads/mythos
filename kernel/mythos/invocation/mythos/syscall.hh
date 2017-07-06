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
 * Copyright 2016 Randolf Rotta, Robert Kuban, and contributors, BTU Cottbus-Senftenberg
 */
#pragma once

#include <cstdint>
#include "mythos/caps.hh"
#include "mythos/Error.hh"

namespace mythos {

  // see I/O Completion Ports like in WinNT, or the Event Completion Framework in Solaris
  // and https://people.freebsd.org/~jlemon/papers/kqueue.pdf
  struct KEvent
  {
    KEvent() : user(0), state(0) {}
    KEvent(uintptr_t user, uint64_t state) : user(user), state(state) {}

    /** A user defined value that identifies the event source.
     * It may, for example, point to a user-space handler object.
     * This value is configured when creating kernel objects that can issue notifications.
     * Passed in register %rax when returning from a system call.
     */
    uintptr_t user;

    /** The status or error code that is returned as event.
     * Passed in register %rdx when returning from a system call.
     */
    uint64_t state;
  };

  enum SyscallCode {
    SYSCALL_EXIT = 0,
    SYSCALL_POLL,
    SYSCALL_WAIT,
    SYSCALL_INVOKE,
    SYSCALL_INVOKE_POLL,
    SYSCALL_INVOKE_WAIT,
    SYSCALL_DEBUG,
    SYSCALL_SIGNAL
  };

  /** do a syscall according to mythos x86-64 system call convention
   *
   * Following registers are preserved across calls: %rsp, %rbp.
   * Following register loose their contents because of HW design: %rax, %rcx, %r11.
   * Arguments are passed in: %rdi (sysno), %rsi (userctx/rescode), %rdx (portal), %r10 (kobj)
   * (could be added if needed: %r8, %r9).
   * Return values are in: %rdi (error code), %rsi (user context).
   * All other registers loose their content by implementation choice.
   */
  inline KEvent syscall(uint64_t sysno, uint64_t a1, uint64_t a2, uint64_t a3)
  {
    register uint64_t r10 asm("r10") = a3;
    asm volatile ("syscall"
      : "+D"(sysno), "+S"(a1), "+d"(a2), "+r"(r10)
      : /* empty */
      : "rax", "rbx", "rcx", "r8", "r9", "r11", "r12", "r13", "r14", "r15", "memory");
    return KEvent(a1, sysno);
  }

  inline void syscall_debug(char const* start, uint64_t length)
  {
    syscall(mythos::SYSCALL_DEBUG, (uint64_t)start, length, 0);
  }

  inline KEvent syscall_poll()
  {
    return syscall(mythos::SYSCALL_POLL, 0, 0, 0);
  }

  inline KEvent syscall_wait()
  {
    return syscall(mythos::SYSCALL_WAIT, 0, 0, 0);
  }

  NORETURN void syscall_exit(uint64_t rescode=0);

  inline void syscall_exit(uint64_t rescode)
  {
    syscall(mythos::SYSCALL_EXIT, rescode, 0, 0);
    while(true);
  }

  inline KEvent syscall_invoke(CapPtr portal, CapPtr object, void* userctx)
  {
    return syscall(mythos::SYSCALL_INVOKE, reinterpret_cast<uintptr_t>(userctx), portal, object);
  }

  inline KEvent syscall_invoke_poll(CapPtr portal, CapPtr object, void* userctx)
  {
    return syscall(mythos::SYSCALL_INVOKE_POLL, reinterpret_cast<uintptr_t>(userctx), portal, object);
  }

  inline KEvent syscall_invoke_wait(CapPtr portal, CapPtr object, void* userctx)
  {
    return syscall(mythos::SYSCALL_INVOKE_WAIT, reinterpret_cast<uintptr_t>(userctx), portal, object);
  }

  inline KEvent syscall_signal(CapPtr ec)
  {
    return syscall(mythos::SYSCALL_SIGNAL, 0, ec, 0);
  }

} // namespace mythos
