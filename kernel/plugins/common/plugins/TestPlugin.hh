/* -*- mode:C++; indent-tabs-mode:nil; -*- */
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
 * Copyright 2017 Randolf Rotta, Robert Kuban, and contributors, BTU Cottbus-Senftenberg
 */
#pragma once

#include "plugins/Plugin.hh"

namespace mythos {

#define TEST_OP(expr, op, val) \
  do {\
    auto result = (expr); \
    auto expected = (val); \
    bool success = ((result) op (expected)); \
    constexpr char expr_str[] = #expr; \
    constexpr char op_str[] = #op; \
    constexpr char val_str[] = #val; \
    test_log(success, expr_str, result, op_str, val_str, expected); \
  } while (false)

#define TEST_EQ(expr, val) TEST_OP(expr, ==, val)
#define TEST_NEQ(expr, val) TEST_OP(expr, !=, val)
#define TEST_SUCCESS(expr) TEST_OP(expr, ==, Error::SUCCESS)
#define TEST_FAILED(expr) TEST_OP(expr, !=, Error::SUCCESS)
#define TEST_TRUE(expr) TEST_OP(expr, ==, true)
#define TEST_FALSE(expr) TEST_OP(expr, !=, true)

class TestPlugin : public Plugin
{
protected:
  TestPlugin(const char* name = "test") : Plugin(name) {}

  void test_success() { _success++; }
  void test_fail() { _failed++; }

  template<class R, class E>
  void test_log(bool success,
                const char* expr_str, R result,
                const char* op_str,
                const char* val_str, E expected);
  void done();

private:
  size_t _success;
  size_t _failed;
};

template<class R, class E>
void TestPlugin::test_log(bool success,
    const char* expr_str, R result,
    const char* op_str,
    const char* val_str, E expected)
{
  if (success) {
    log.info("PASSED", expr_str, op_str, val_str);
    test_success();
  } else {
    log.error("FAILED", expr_str, op_str, val_str);
    log.info("VALUES", result, op_str, expected);
    test_fail();
  }
}

void TestPlugin::done()
{
  auto tests = _failed + _success;
  if (!_failed) {
    log.error("SUCCESS", tests, "tests have passed");
  } else {
    log.error("FAILED", _success, "out of", tests, "tests have passed");
  }
}

} // namespace mythos
