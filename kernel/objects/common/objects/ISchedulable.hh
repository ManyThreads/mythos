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

    /** Checks if the Execution Context could be runnable.
     * This can be used to avoid loading ECs that fail to resume anyway.
     * The EC's state can change concurrently but the actual resume() will solve this.
     */
    virtual bool isReady() const = 0;

    /** If there is no reason to be blocked, loads the thread's state and 
     * switches to user-mode without ever coming back. Otherwise, 
     * if the execution is blocked it returns.
     * 
     * It has to be called with the same execution context already loaded
     * from a previous resume() or with clear current_ec, thread_state, 
     * and currentPlace. Clearing is achieved by calling saveState() at
     * the hardware thread where the execution context is currently loaded.
     *
     * There would be a race between seeing that the thread is not blocked
     * and blocking the thread from the outside. This is solved by resume().
     * It is called after the kernel released its task queues to ensure that
     * any blocking cause will also preempt the hardware thread. And then resume()
     * checks that it is actually still ready. Otherwise it returns.
     */
    virtual void resume() = 0;

    virtual void loadState() = 0;
    
    virtual void saveState() = 0;

    virtual bool hasPriority() { return false; } // no high priority per default

    /** informs the execution context that the user-mode code trapped and error handling is needed. */
    virtual void handleTrap() = 0;

    /** informs the execution context that it was interrupted. */
    virtual void handleInterrupt() = 0;

    /** lets the execution context process the current system call. */ 
    virtual void handleSyscall() = 0;
  };

    /** The Execution Context that is currently loaded on each hardware thread. 
    * This variable is updated only by the hardware thread's local scheduler or
    * by the execution context's resume method but never remotely.
    * It is used from the kernel entry points to handle system calls and traps.
    * 
    * @todo is the atomic really needed?
    */
    extern CoreLocal<std::atomic<ISchedulable*>> current_ec KERNEL_CLM_HOT;

    /** Forward the trap handling to the currently loaded execution context.
    * If current_ec is nullptr than we can not return from user space because we never left.
    */
    inline void ec_handle_trap() {
        ASSERT(current_ec->load() != nullptr);
        current_ec->load()->handleTrap();
    }
    
    /** Forward the system call handling to the currently loaded execution context.
    * If current_ec is nullptr than we can not return from user space because we never left.
    */
    inline void ec_handle_syscall() {
        ASSERT(current_ec->load() != nullptr);
        current_ec->load()->handleSyscall();
    }

    inline void ec_interrupted() {
        auto ec = current_ec->load();
        if (ec) ec->handleInterrupt();
    }
} // namespace mythos
