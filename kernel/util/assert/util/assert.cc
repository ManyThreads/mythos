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
#include "util/assert.hh"
#include "util/Logger.hh"
#include "cpu/hwthread_pause.hh"
#include "cpu/stacktrace.hh"

extern "C" NORETURN bool __assert_fail(char const* file, unsigned int line, const char* expr, char const * message) {
  mlog::Logger<> logassert("assert");
  if (message)
    logassert.error("ASSERTION in",file,":",line,"failed:",expr,"-",message);
  else
    logassert.error("ASSERTION in",file,":",line,"failed:",expr);
  for (auto frame : mythos::StackTrace()) { logassert.info("trace", frame.ret); }
  mythos::sleep_infinitely();
}

bool __oops_fail(char const* file, unsigned int line, const char* expr, char const * message) {
  mlog::Logger<> logassert("assert");
  if (message)
    logassert.error("KERNEL OOPS in",file,":",line,"failed:",expr,"-",message);
  else
    logassert.error("KERNEL OOPS in",file,":",line,"failed:",expr);
  for (auto frame : mythos::StackTrace()) { logassert.info("trace", frame.ret); }
  return true;
}

extern "C" NORETURN bool __panic_fail(char const* file, unsigned int line, const char* expr, char const * message) {
  mlog::Logger<> logassert("assert");
  if (message)
    logassert.error("KERNEL PANIC in",file,":",line,"failed:",expr,"-",message);
  else
    logassert.error("KERNEL PANIC in",file,":",line,"failed:",expr);
  for (auto frame : mythos::StackTrace()) { logassert.info("trace", frame.ret); }
  mythos::sleep_infinitely();
}

