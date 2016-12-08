#pragma once

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
