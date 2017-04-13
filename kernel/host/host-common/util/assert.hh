#pragma once

// note: the mythos host should not depend on the implementation in base/assert.cc
// but we can add host/assert.cc one day...
#include <assert.h>
#define ASSERT(expr) assert(expr)
#define ASSERT_MSG(expr, message) assert(expr)
#define OOPS(expr) assert(expr)
#define OOPS_MSG(expr, message) assert(expr)
#define PANIC(expr) assert(expr)
#define PANIC_MSG(expr, message) assert(expr)
