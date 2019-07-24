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

    bool isParentOf(CapEntry& parentEntry, Cap parent, CapEntry& otherEntry, Cap other);

    template<typename COMMITFUN>
    optional<void> inherit(CapEntry& parentEntry, Cap parentCap, 
                           CapEntry& targetEntry, Cap targetCap,
                           COMMITFUN const& commit);

    inline optional<void> inherit(CapEntry& parentEntry, Cap parentCap,
                           CapEntry& targetEntry, Cap targetCap)
    {
        return inherit(parentEntry, parentCap, targetEntry, targetCap, [](){});
    }

    optional<void> derive(CapEntry& parentEntry, CapEntry& targetEntry,
        Cap parentCap, CapRequest request = 0);

    optional<void> reference(CapEntry& parentEntry, CapEntry& targetEntry,
        Cap parentCap, CapRequest request = 0);

    template<class COMMITFUN>
    optional<void> setReference(CapEntry& dst, Cap dstCap, CapEntry& src, Cap srcCap, const COMMITFUN& fun)
    {
      ASSERT(dstCap.isReference());
      if (!dst.acquire()) THROW(Error::LOST_RACE);
      RETURN(inherit(src, srcCap, dst, dstCap, [=](){ fun(); }));
    }

    template<class COMMITFUN>
    bool resetReference(CapEntry& dst, const COMMITFUN& fun)
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

    inline bool resetReference(CapEntry& dst) { return resetReference(dst, [](){}); }

    template<typename COMMITFUN>
    optional<void> inherit(CapEntry& parentEntry, Cap parentCap, 
                           CapEntry& targetEntry, Cap targetCap,
                           COMMITFUN const& commit)
    {
      MLOG_DETAIL(mlog::cap, "cap::inherit", DVAR(parentCap), DVAR(targetCap));
      auto result = parentEntry.insertAfter(parentCap, targetEntry, targetCap, 
        [&](){
          commit(); // exec the higher level commit action while everything is locked and successful

          //auto parentRange = parentCap.getPtr()->addressRange(parentEntry, parentCap);
          //auto targetRange = targetCap.getPtr()->addressRange(targetEntry, targetCap);
          //MLOG_DETAIL(mlog::cap, DVAR(parentRange), DVAR(targetRange));

          // now we can check whether the tree is still valid. Cannot be done outside the locks!
          ASSERT(implies(parentCap.isReference(), !isParentOf(parentEntry, parentCap, targetEntry, targetCap) 
                                             && !isParentOf(targetEntry, targetCap, parentEntry, parentCap)));
          ASSERT(implies(!parentCap.isReference(), isParentOf(parentEntry, parentCap, targetEntry, targetCap)));
        });
      RETURN(result);
    }

  } // namespace cap
} // namespace mythos
