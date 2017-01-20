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
 * Copyright 2016 Randolf Rotta, Robert Kuban, and contributors, BTU Cottbus-Senftenberg
 */

#include "objects/ops.hh"
#include "objects/CapEntry.hh"
#include "mythos/caps.hh"
#include "objects/IKernelObject.hh"

namespace mythos {
  namespace cap {

  bool isParentOf(Cap parent, Cap other)
  {
    ASSERT(parent.getPtr());
    ASSERT(other.getPtr());
    // first check ranges
    auto parentRange = parent.getPtr()->addressRange(parent);
    auto otherRange = other.getPtr()->addressRange(other);
    //MLOG_DETAIL(mlog::cap, DVAR(parentRange), DVAR(otherRange));
    if (parentRange.contains(otherRange)) {

      if (!otherRange.contains(parentRange)) {
        // parent range is real superset
        // so it's a child if parent is not a reference
        return !parent.isReference();
      }

      // check the flags
      if (parent.isOriginal()) {
        ASSERT_MSG(!other.isDerivedReference(), "there must be a derived cap inbetween");
        return other.isReference() || other.isDerived();
      } else if (parent.isDerived() && !parent.isReference()) {
        return other.isDerivedReference();
      } else {
        ASSERT(parent.isReference());
        return false; // References can only have siblings
      }
    }
    return false; // is not contained
  }

  optional<void> inherit(CapEntry& thisEntry, CapEntry& newEntry, Cap thisCap, Cap newCap)
  {
    MLOG_DETAIL(mlog::cap, "inherit caps", DVAR(thisCap), DVAR(newCap),
		    DVAR(isParentOf(thisCap, newCap)), DVAR(isParentOf(newCap, thisCap)));
    ASSERT(newEntry.cap().isAllocated());
    ASSERT(implies(thisCap.isReference(), !isParentOf(thisCap, newCap) && !isParentOf(newCap, thisCap)));
    ASSERT(implies(!thisCap.isReference(), isParentOf(thisCap, newCap)));  
    // try to insert into the resource tree
    // insertAfter checks if thisCap is stored in thisEntry after locking
    // and fails if it is not ...
    auto result = thisEntry.insertAfter(thisCap, newEntry);
    if (result) newEntry.commit(newCap); // release the entry as usable
    else newEntry.reset(); // release exclusive usage and revert to an empty entry
    return result;
  }

  optional<void> derive(CapEntry& thisEntry, CapEntry& newEntry, Cap thisCap, CapRequest request)
  {
    if (!thisCap.isUsable() || thisCap.isDerived()) return Error::INVALID_CAPABILITY;
    auto minted = thisCap.getPtr()->mint(thisCap, request, true);
    if (!minted) return minted.state();
    if (!newEntry.acquire()) return Error::LOST_RACE; // try to acquire exclusive usage
    return inherit(thisEntry, newEntry, thisCap, (*minted).stripReference().asDerived());
  }

  optional<void> reference(CapEntry& thisEntry, CapEntry& newEntry, Cap thisCap, CapRequest request)
  {
    if (!thisCap.isUsable()) return Error::INVALID_CAPABILITY;
    auto minted = thisCap.getPtr()->mint(thisCap, request, false);
    if (!minted) return minted.state();
    if (!newEntry.acquire()) return Error::LOST_RACE; // try to acquire exclusive usage
    return inherit(thisEntry, newEntry, thisCap, (*minted).asReference());
  }

  } // namespace cap
} // namespace mythos
