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
 * Copyright 2016 Randolf Rotta, BTU Cottbus-Senftenberg
 */
#pragma once

#include <cstdint>

namespace mythos {

  enum struct Error : uint8_t
  {
    SUCCESS = 0,
    UNSET = 1,    // the error value never has been set, do not use as return value
    INHIBIT,      // not really an error, just tells that result will be sent later
    GENERIC_ERROR,
    INVALID_CAPABILITY,
    INVALID_ARGUMENT,
    NON_CANONICAL_ADDRESS,
    UNALIGNED,
    INSUFFICIENT_RESOURCES,
    LOST_RACE,
    RETRY,
    NO_MESSAGE,
    TYPE_MISMATCH,
    NOT_IMPLEMENTED,
    CAP_NONEMPTY,
    NOT_KERNELMEM,
    CYCLIC_DEPENDENCY,
    PORTAL_NOT_OPEN,
    PORTAL_NOT_INVOKED,
    PORTAL_NO_BUFFER,
    PORTAL_NO_ENDPOINT,
    NO_LOOKUP,
    INVALID_REQUEST,
    REQUEST_DENIED,
    PAGEMAP_MISSING,
    PAGEMAP_NOCONF
  };

} // namespace mythos
