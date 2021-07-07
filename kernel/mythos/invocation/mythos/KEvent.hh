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
 * Copyright 2021 Randolf Rotta, Robert Kuban, and contributors, BTU Cottbus-Senftenberg
 */
#pragma once

#include <cstdint>

namespace mythos {


// see I/O Completion Ports like in WinNT, or the Event Completion Framework in Solaris
// and https://people.freebsd.org/~jlemon/papers/kqueue.pdf
struct KEvent
{
    typedef uintptr_t Context;
    typedef uint64_t Value;

    KEvent() : user(0), state(0) {}
    KEvent(Context user, Value state) : user(user), state(state) {}

    /** A user defined value that identifies the event source.
     * It may, for example, point to a user-space handler object.
     * This value is configured when creating kernel objects that can issue notifications.
     * Passed in register %rax when returning from a system call.
     */
    Context user;

    /** The status or error code that is returned as event.
     * Passed in register %rdx when returning from a system call.
     */
    Value state;
};

} // namespace mythos
