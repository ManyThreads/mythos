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

#include "objects/ops.hh"
#include "objects/CapEntry.hh"
#include "mythos/caps.hh"
#include "objects/IKernelObject.hh"
#include "util/error-trace.hh"

namespace mythos {
  namespace cap {

  bool isParentOf(CapEntry& parentEntry, Cap parent, CapEntry& otherEntry, Cap other)
  {
    ASSERT(parent.getPtr());
    ASSERT(other.getPtr());
    // first check ranges
    auto parentRange = parent.getPtr()->addressRange(parentEntry, parent);
    auto otherRange = other.getPtr()->addressRange(otherEntry, other);
    //MLOG_DETAIL(mlog::cap, DVAR(parentRange), DVAR(otherRange));
    if (!parentRange.contains(otherRange)) return false; // is not within parent's range
    if (!otherRange.contains(parentRange)) return !parent.isReference(); // subrange is child unless parent is reference

    // both cover exactly the same range
    if (parent.isReference()) return false; // references can only have siblings, this also covers parent.isDerivedReverence()
    if (parent.isDerived()) return other.isDerivedReference(); // derived can have only derived references as child

    // parent is an original
    ASSERT_MSG(!other.isDerivedReference(), "there must be a derived cap inbetween");
    return true; // everything with equal range to an orignial is a child
  }

  optional<void> derive(CapEntry& parentEntry, CapEntry& targetEntry, Cap parentCap, CapRequest request)
  {
    if (!parentCap.isUsable() || parentCap.isDerived()) THROW(Error::INVALID_CAPABILITY);
    auto minted = parentCap.getPtr()->mint(parentEntry, parentCap, request, true);
    if (!minted) RETHROW(minted);
    if (!targetEntry.acquire()) THROW(Error::LOST_RACE); // try to acquire exclusive usage
    RETURN(inherit(parentEntry, parentCap, targetEntry, (*minted).stripReference().asDerived()));
  }

  optional<void> reference(CapEntry& parentEntry, CapEntry& targetEntry, Cap parentCap, CapRequest request)
  {
    if (!parentCap.isUsable()) THROW(Error::INVALID_CAPABILITY);
    auto minted = parentCap.getPtr()->mint(parentEntry, parentCap, request, false);
    if (!minted) RETHROW(minted);
    if (!targetEntry.acquire()) THROW(Error::LOST_RACE); // try to acquire exclusive usage
    RETURN(inherit(parentEntry, parentCap, targetEntry, (*minted).asReference()));
  }

  } // namespace cap
} // namespace mythos
