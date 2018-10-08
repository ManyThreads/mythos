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
#include "util/assert.hh"
#include "util/Logger.hh"
#include "mythos/syscall.hh"

extern "C" NORETURN bool massert_fail(char const* file, unsigned int line, const char* expr, char const * message) {
  mlog::Logger<> logassert("assert");
  if (message)
    logassert.error("USER ASSERTION in",file,":",line,"failed:",expr,"-",message);
  else
    logassert.error("USER ASSERTION in",file,":",line,"failed:",expr);
  mythos::syscall_exit(-1); /// @TODO syscall_abort(); to see some stack backtrace etc
}

bool moops_fail(char const* file, unsigned int line, const char* expr, char const * message) {
  mlog::Logger<> logassert("assert");
  if (message)
    logassert.error("USER OOPS in",file,":",line,"failed:",expr,"-",message);
  else
    logassert.error("USER OOPS in",file,":",line,"failed:",expr);
  return true;
}

extern "C" NORETURN bool mpanic_fail(char const* file, unsigned int line, const char* expr, char const * message) {
  mlog::Logger<> logassert("assert");
  if (message)
    logassert.error("USER PANIC in",file,":",line,"failed:",expr,"-",message);
  else
    logassert.error("USER PANIC in",file,":",line,"failed:",expr);
  mythos::syscall_exit(-1); /// @TODO syscall_abort();
}

