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
 * Copyright 2016 Robert Kuban, Randolf Rotta, and contributors, BTU Cottbus-Senftenberg
 */

#include "objects/CapEntry.hh"
#include "objects/mlog.hh"
#include "util/error-trace.hh"

namespace mythos {

  void CapEntry::initRoot(Cap c)
  {
    MLOG_ERROR(mlog::cap, __PRETTY_FUNCTION__, DVAR(this));
    ASSERT(isKernelAddress(this));
    ASSERT(c.isUsable());
    ASSERT(cap().isEmpty());
    Link loopLink(this);
    MLOG_ERROR(mlog::cap, "this unlocks _next");
    _next.store(loopLink.value());
    MLOG_ERROR(mlog::cap, "this unlocks _prev");
    _prev.store(loopLink.value());
    _cap.store(c.value());
  }

  bool CapEntry::tryAcquire()
  {
    auto expected = Cap::asEmpty().value();
    const auto desired = Cap::asAllocated().value();
    return _cap.compare_exchange_strong(expected, desired);
  }

  optional<void> CapEntry::acquire()
  {
    if (tryAcquire()) RETURN(Error::SUCCESS);
    else THROW(Error::CAP_NONEMPTY);
  }

  void CapEntry::commit(const Cap& cap)
  {
    ASSERT(isLinked());
    _cap.store(cap.value());
  }

  void CapEntry::reset()
  {
    MLOG_ERROR(mlog::cap, __PRETTY_FUNCTION__, DVAR(this));
    ASSERT(isUnlinked() || cap().isAllocated());
    _prev.store(Link().value());
    _next.store(Link().value());
    // mark as empty
    _cap.store(Cap().value());
  }

  void CapEntry::setPrevPreserveFlags(CapEntry* ptr)
  {
    auto expected = _prev.load();
    uintlink_t desired;
    do {
      desired = Link(expected).withPtr(ptr).value();
    } while (!_prev.compare_exchange_weak(expected, desired));
  }

  optional<void> CapEntry::moveTo(CapEntry& other)
  {
    MLOG_ERROR(mlog::cap, __PRETTY_FUNCTION__, DVAR(this), DVAR(other));
    ASSERT(other.cap().isAllocated());
    ASSERT(!other.isLinked());
    lock_cap();
    if (!lock_prev()) {
      unlock_cap();
      other.reset();
      THROW(Error::GENERIC_ERROR);
    }
    lock_next();
    auto thisCap = cap();
    if (isRevoking() || !thisCap.isUsable()) {
      other.reset();
      unlock_next();
      unlock_prev();
      unlock_cap();
      THROW(Error::INVALID_CAPABILITY);
    }

    // using these values removes lock
    auto next= Link(_next).withoutFlags();
    auto prev= Link(_prev).withoutFlags();

    next->setPrevPreserveFlags(&other);
    other._next.store(next.value());
    // deletion, deleted or revoking can not be set in other._prev
    // as we allocated other for moving
    other._prev.store(prev.value());
    MLOG_ERROR(mlog::cap, "this unlocks prev");
    prev->_next.store(Link(&other).value());
    other.commit(thisCap);
    MLOG_ERROR(mlog::cap, "this unlocks cap");
    _prev.store(Link().value());
    MLOG_ERROR(mlog::cap, "this unlocks next");
    _next.store(Link().value());
    _cap.store(Cap().value());
    RETURN(Error::SUCCESS);
  }

  bool CapEntry::kill()
  {
    auto expected = _cap.load();
    Cap curCap;
    do {
      curCap = Cap(expected);
      MLOG_DETAIL(mlog::cap, this, ".kill", DVAR(curCap));
      if (!curCap.isUsable()) {
        return curCap.isZombie() ? true : false;
      }
    } while (!_cap.compare_exchange_strong(expected, curCap.asZombie().value()));
    return true;
  }

  bool CapEntry::kill(Cap expected)
  {
    CapValue expectedValue = expected.value();
    MLOG_DETAIL(mlog::cap, this, ".kill", DVAR(expected));
    return _cap.compare_exchange_strong(expectedValue, expected.asZombie().value());
  }


  optional<void> CapEntry::unlinkAndUnlockLinks()
  {
    MLOG_ERROR(mlog::cap, __PRETTY_FUNCTION__, DVAR(this));
    auto next = Link(_next);
    auto prev = Link(_prev);
    next->setPrevPreserveFlags(prev.ptr());
    MLOG_ERROR(mlog::cap, "this unlocks _next of predecessor");
    prev->_next.store(next.withoutFlags().value());
    _prev.store(Link().withoutPtr().value());
    MLOG_ERROR(mlog::cap, "this unlocks _next");
    _next.store(Link().value());
    RETURN(Error::SUCCESS);
  }

  Error CapEntry::try_lock_prev()
  {
    auto prev = Link(_prev).ptr();
    MLOG_ERROR(mlog::cap, __PRETTY_FUNCTION__, DVAR(this), DVAR(prev));
    if (!prev) {
      return Error::GENERIC_ERROR;
    }
    if (prev->try_lock_next()) {
      if (Link(_prev.load()).ptr() == prev) {
        return Error::SUCCESS;
      } else { // my _prev has changed in the mean time
        MLOG_ERROR(mlog::cap, "this unlocks prev");
        prev->unlock_next();
        return Error::RETRY;
      }
    } else return Error::RETRY;
  }

  bool CapEntry::lock_prev()
  {
    Error result;
    for (result = try_lock_prev(); result == Error::RETRY; result = try_lock_prev()) {
      hwthread_pause();
    }
    return result == Error::SUCCESS;
  }

} // namespace mythos
