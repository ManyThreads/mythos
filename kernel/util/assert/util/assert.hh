/* -*- mode:C++; -*- */
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
#pragma once

#include "util/compiler.hh"

inline bool implies(const bool& a, const bool& b)
{
  return !a || b;
}

/** write an error message and stops execution forever.
 */
extern "C" NORETURN bool __assert_fail(char const* file, unsigned int line,
            char const* expr, char const* message=0);

// only in debug mode: if the expression evaluates to false, the programm will be stopped
#ifndef NDEBUG
#define ASSERT(expr) \
  ((void)(!(expr) && __assert_fail(__FILE__,__LINE__, #expr)))
#define ASSERT_MSG(expr, message) \
  ((void)(!(expr) && __assert_fail(__FILE__,__LINE__, #expr, message)))
#else /* not def NDEBUG */
#define ASSERT(expr) (void)(expr)
#define ASSERT_MSG(expr, message) (void)(expr)
#endif

/** write an error message but does not stop.
 */
extern "C" bool __oops_fail(char const* file, unsigned int line,
          char const* expr, char const* message=0);

// the operating system detected an internal error but tries to ignore it
#define OOPS(expr) \
  ((void)(!(expr) && __oops_fail(__FILE__,__LINE__, #expr)))
#define OOPS_MSG(expr, message) \
  ((void)(!(expr) && __oops_fail(__FILE__,__LINE__, #expr, message)))

/** write an error message and stop execution.
 */
extern "C" NORETURN bool __panic_fail(char const* file, unsigned int line,
    char const* expr, char const* message=0);

// the operating system detected an internal fatal error
// from which it cannot safely recover, stops execution
#define PANIC(expr) \
  ((void)(!(expr) && __panic_fail(__FILE__,__LINE__, #expr)))
#define PANIC_MSG(expr, message) \
  ((void)(!(expr) && __panic_fail(__FILE__,__LINE__, #expr, message)))

