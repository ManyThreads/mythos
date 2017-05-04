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
#pragma once

#include "util/ISink.hh"
#include "util/TextMsg.hh"
#include "util/Filter.hh"
#include "util/compiler.hh"

namespace mlog {

extern ISink* sink;

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define PATH "[" __FILE__ ":" TOSTRING(__LINE__) "]"

#define MLOG_DETAIL(logger, ...) \
  if ((logger).isActive(mlog::TextDetail::VERBOSITY)) logger.write<mlog::TextDetail>( \
          PATH, __VA_ARGS__);
#define MLOG_INFO(logger, ...) \
  if ((logger).isActive(mlog::TextInfo::VERBOSITY)) logger.write<mlog::TextInfo>( \
         PATH, __VA_ARGS__);
#define MLOG_WARN(logger, ...) \
  if ((logger).isActive(mlog::TextWarning::VERBOSITY)) logger.write<mlog::TextWarning>( \
          PATH, __VA_ARGS__);
#define MLOG_ERROR(logger, ...) \
  if ((logger).isActive(mlog::TextError::VERBOSITY)) logger.write<mlog::TextError>( \
          PATH, __VA_ARGS__);

template<class Filter = FilterAny>
class Logger
  : public Filter
{
public:
  Logger(char const* name = "mlog") : name(name) {}
  Logger(char const* name, Filter filter) : Filter(filter), name(name) {}
  Logger(Filter filter) : Filter(filter), name("mlog") {}
  void setName(char const* name) { this->name = name; }

  template<class MSG, typename... ARGS>
  void log(ARGS&&... args) const {
    if (this->isActive(MSG::VERBOSITY)) write<MSG, ARGS...>(std::forward<ARGS>(args)...);
  }

  template<class MSG, typename... ARGS>
  void write(ARGS&&... args) const NOINLINE;

  void flush() { sink->flush(); }

  // syntactic sugar
  template<typename... ARGS>
  void detail(ARGS&&... args) { log<TextDetail>(std::forward<ARGS>(args)...); }

  template<typename... ARGS>
  void info(ARGS&&... args) { log<TextInfo>(std::forward<ARGS>(args)...); }

  template<typename... ARGS>
  void warn(ARGS&&... args) { log<TextWarning>(std::forward<ARGS>(args)...); }

  template<typename... ARGS>
  void error(ARGS&&... args) { log<TextError>(std::forward<ARGS>(args)...); }

  template<typename... ARGS>
  void csv(ARGS&&... args) { log<TextCSV>(std::forward<ARGS>(args)...); }

protected:
  char const* name;
};

template<class Filter>
template<class MSG, typename... ARGS>
void Logger<Filter>::write(ARGS&&... args) const {
  MSG msg(name, std::forward<ARGS>(args)...);
  sink->write(msg.getData(), msg.getSize());
}

static Logger<FilterAny> testLog("Test");
#define MLOG_TEST(logger, expr) do { \
    if (expr) \
      logger.write<mlog::TextSuccess>(PATH, "Success:", TOSTRING(expr)); \
    else \
      logger.write<mlog::TextFailure>(PATH, "Fail:", TOSTRING(expr)); \
    } while(0)

#define MLOG_TEST_OP(logger, op, val, expected) \
    do { \
    if ((val) op (expected)) \
      logger.write<mlog::TextSuccess>(PATH, "Success:", TOSTRING(val), TOSTRING(op), TOSTRING(expected)); \
    else \
      logger.write<mlog::TextFailure>(PATH, "Fail:", TOSTRING(val), TOSTRING(op), TOSTRING(expected)); \
    } while(0)
#define MLOG_TEST_EQ(logger, val, expected) MLOG_TEST_OP(logger, ==, val, expected)
#define MLOG_TEST_NEQ(logger, val, expected) MLOG_TEST_OP(logger, !=, val, expected)
#define MLOG_TEST_TRUE(logger, expr) MLOG_TEST_OP(logger, ==, expr, true)
#define MLOG_TEST_FALSE(logger, expr) MLOG_TEST_OP(logger, !=, expr, true)

#define TEST(expr) MLOG_TEST(mlog::testLog, expr)
#define TEST_EQ(val, expected) MLOG_TEST_OP(mlog::testLog, ==, val, expected)
#define TEST_NEQ(val, expected) MLOG_TEST_OP(mlog::testLog, !=, val, expected)
#define TEST_TRUE(expr) MLOG_TEST_OP(mlog::testLog, ==, expr, true)
#define TEST_FALSE(expr) MLOG_TEST_OP(mlog::testLog, !=, expr, true)
#define TEST_SUCCESS(expr) MLOG_TEST_OP(mlog::testLog, ==, expr, Error::SUCCESS)
#define TEST_FAILED(expr) MLOG_TEST_OP(mlog::testLog, !=, expr, Error::SUCCESS)

} // namespace mlog
