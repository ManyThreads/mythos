/* -*- mode:C++; indent-tabs-mode:nil; -*- */
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

#include "cpu/CoreLocal.hh"
#include <atomic>

namespace mythos {
  namespace cpu {
    struct ThreadState;
  } // namespace cpu;

  class ISchedulable
  {
  public:
    virtual ~ISchedulable() {}

    /** Checks if the Execution Context can be loaded and resumed currently.
     * This can be used to avoid loading ECs that are not executable anyway.
     * The EC's state can change concurrently but the actual resume() will solve this.
     */
    virtual bool isReady() const = 0;

    /** tries to switch to user-mode execution if there is no reason
     * to be blocked and return otherwise.
     *
     * We do not check if the loaded EC is runnable. Instead just call
     * resume() and it will run if possible. In that case control does
     * not return from this function. Otherwise, the call will return.
     */
    virtual void resume() = 0;

    virtual void handleTrap(cpu::ThreadState* ctx) = 0;

    virtual void handleSyscall(cpu::ThreadState* ctx) = 0;

    virtual void unload() = 0;

    virtual void semaphoreNotify() = 0;
  };

  extern CoreLocal<std::atomic<ISchedulable*>> current_ec KERNEL_CLM_HOT;

  inline void handle_trap(cpu::ThreadState* ctx) { current_ec->load()->handleTrap(ctx); }
  inline void handle_syscall(cpu::ThreadState* ctx) { current_ec->load()->handleSyscall(ctx); }

} // namespace mythos
