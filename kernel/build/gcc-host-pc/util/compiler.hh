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

//[[gnu::aligned(X)]]
//__attribute__ ((aligned(__alignof__(T))))
#define ALIGNED(X)          alignas(X)
#define ALIGN_4K            alignas(4096)
#define ALWAYS_INLINE       __attribute__((always_inline))
#define INIT                __attribute__((section (".init")))
#define INITDATA            __attribute__((section (".initdata")))
#define NORETURN            [[noreturn]]
//__attribute__((noreturn))
#define NOINLINE            __attribute__((noinline))
#define REGPARM(X)          __attribute__((regparm(X)))
#define PACKED              [[gnu::packed]]
// __attribute__((packed))
#define PURE                __attribute__((pure))
#define SYMBOL(x)		asm(x)

#define CONSTEXPR constexpr
#define NOEXCEPT noexcept
#define WARN_UNUSED __attribute__((warn_unused_result))
