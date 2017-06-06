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
 * Copyright 2016 Randolf Rotta, Robert Kuban, and contributors, BTU Cottbus-Senftenberg
 */
#pragma once

#include "objects/Cap.hh"
#include "objects/CapEntry.hh"
#include "util/optional.hh"
#include "util/error-trace.hh"

namespace mythos {

  namespace cap {

    bool isParentOf(Cap parent, Cap other);

    optional<void> inherit(CapEntry& thisEntry, CapEntry& newEntry, Cap thisCap, Cap newCap);

    optional<void> derive(CapEntry& thisEntry, CapEntry& newEntry,
        Cap thisCap, CapRequest request = 0);

    optional<void> reference(CapEntry& thisEntry, CapEntry& newEntry,
        Cap thisCap, CapRequest request = 0);

    template<class FUN>
    optional<void> setReference(const FUN fun, CapEntry& dst, Cap dstCap, CapEntry& src, Cap srcCap)
    {
      ASSERT(dstCap.isReference());
      if (!dst.acquire()) THROW(Error::LOST_RACE);
      // was empty => insert new reference
      fun();
      RETURN(inherit(src, dst, srcCap, dstCap));
    }

    template<class FUN>
    bool resetReference(const FUN fun, CapEntry& dst)
    {
      if (!dst.kill()) return false; // not killable (allocated but not usable)
      if (!dst.lock_prev()) return true; // was already unlinked, should be empty eventually
      dst.lock();
      dst.kill(); // kill again because someone might have inserted something usable meanwhile
      dst.unlink();
      fun(); // perform some caller-defined action while still in an exclusive state
      dst.reset(); // this markes the entry as writeable again, leaves exclusive state
      return true;
    }

    inline bool resetReference(CapEntry& dst) { return resetReference([]{}, dst); }

  } // namespace cap
} // namespace mythos
