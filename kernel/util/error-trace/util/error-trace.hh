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

#include "util/Logger.hh"
#include "util/assert.hh"
#include "cpu/stacktrace.hh"
#include "util/optional.hh"

namespace mlog {

#ifndef MLOG_ERROR_THROW
#define MLOG_ERROR_THROW FilterAny
#endif
  extern Logger<MLOG_ERROR_THROW> throw_error;

} //  mlog

#define THROW(...) do { return ::mythos::throw_error(PATH, __VA_ARGS__);} while (false)
#define RETHROW(...) do { return ::mythos::rethrow_error(PATH, __VA_ARGS__); } while (false)

#ifndef NDEBUG
#define RETURN(value) \
  do { \
    auto val = ::mythos::boxError(value); \
    if (val.state() == ::mythos::Error::SUCCESS || val.state() == ::mythos::Error::INHIBIT) {\
      return val; \
    } \
    RETHROW(val); \
  } while (false)
#else
#define RETURN(value) do { return ::mythos::boxError(value); } while (false)
#endif


namespace mythos {

#ifndef NDEBUG
  template<class... ARGS>
  inline optional<void> throw_error(const char* path, Error error, ARGS... args)
  {
    mlog::throw_error.error(path, DVAR(error), args...);
    for (auto frame : mythos::StackTrace()) { mlog::throw_error.detail("trace", frame.ret); }
    return optional<void>(error);
  }
#else
  template<class... ARGS>
  inline optional<void> throw_error(const char*, Error error, ARGS...)
  {
    return optional<void>(error);
  }
#endif


#ifndef NDEBUG
  template<class... ARGS>
  inline optional<void> rethrow_error(const char* path, Error error, ARGS... args)
  {
    mlog::throw_error.warn(path, DVAR(error), args...);
    return optional<void>(error);
  }
#else
  template<class... ARGS>
  inline optional<void> rethrow_error(const char*, Error error, ARGS...)
  {
    return optional<void>(error);
  }
#endif

  template<class OPT, class... ARGS>
  inline optional<void> rethrow_error(const char* path, OPT opt, ARGS... args)
  {
    return rethrow_error(path, opt.state(), args...);
  }

}
