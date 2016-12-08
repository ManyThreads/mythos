#pragma once

#include "objects/mlog.hh"
#include "util/Plugin.hh"

namespace mythos {

class TestPlugin : public Plugin
{
  protected:
    TestPlugin(const char* name) : _logger(name) {}
    void test_success() { _success++; }
    void test_fail() { _failed++; }
    template<class R, class E>
      void test_log(bool success,
          const char* expr_str, R result,
          const char* op_str,
          const char* val_str, E expected);
    void done();
    mlog::Logger<mlog::FilterAny> _logger;
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
      _logger.info("PASSED", expr_str, op_str, val_str);
      test_success();
    } else {
      _logger.error("FAILED", expr_str, op_str, val_str);
      _logger.info("VALUES", result, op_str, expected);
      test_fail();
    }
}

void TestPlugin::done()
{
  auto tests = _failed + _success;
  if (!_failed) {
    _logger.error("SUCCESS", tests, "tests have passed");
  } else {
    _logger.error("FAILED", _success, "out of", tests, "tests have passed");
  }
}

}
