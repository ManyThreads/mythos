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
#pragma once

#include "objects/mlog.hh"
#include <cstdint>
#include <atomic>
#include "util/optional.hh"
#include "objects/Cap.hh"
#include "cpu/hwthread_pause.hh"
#include "util/assert.hh"
#include "util/error-trace.hh"

namespace mythos {

  class IKernelObject;

  /* blocking doubly linked list
   * locking policy: lock from left to right, unlock from right to left
   * no dining philosophers as root does never lock itself,
   * thus prev and next of root are locked independently
   *
   * operations that write to the capability
   * or flags (zombie) must lock both next and prev
   */
  class CapEntry
  {
  public:
    typedef CapValue uintcap_t;
    typedef uint32_t uintlink_t;

    CapEntry(Cap cap) : _cap(cap.value()), _prev(0), _next(0) {}
    CapEntry() : _cap(Cap().value()), _prev(0), _next(0) {}

    Cap cap() const { return Cap(_cap.load()); }
    void initRoot(Cap c);

    /** try to insert a child behind this parent.
     * Uses lazy-locking: after locking the parent-child link, it is checked that 
     * the parent entry still contains the same capability and is still usable, i.e. not deleted.
     */
    template<typename COMMITFUN>
    optional<void> insertAfter(Cap parentCap, CapEntry& targetEntry, Cap targetCap, COMMITFUN const& commit);
    
    optional<void> moveTo(CapEntry& other);

    bool setRevoking() { return !(_prev.fetch_or(REVOKING_FLAG) & REVOKING_FLAG); }
    void finishRevoke() { _prev.fetch_and(~REVOKING_FLAG); }
    bool isRevoking() const { return _prev.load() & REVOKING_FLAG; }

    void setDeleted() { _prev.fetch_or(DELETED_FLAG); }
    bool isDeleted() const { return _prev.load() & DELETED_FLAG; }

    bool tryAcquire();
    optional<void> acquire();
    void commit(const Cap& cap);
    void reset();

    /** Bring entry into zombi state. Retries until zombified, empty or
     * allocated. Returns true if zombified. */ 
    bool kill();

    optional<void> unlinkAndUnlockPrev();

    /* lock next functions protect the link to the next CapEntry */

    bool try_lock_next()
    { 
      bool ret = !(_next.fetch_or(LOCKED_FLAG) & LOCKED_FLAG);
      MLOG_ERROR(mlog::cap, __PRETTY_FUNCTION__, DVAR(this), ret? " locked" : "locking failed!");
      return ret;
    }

    void lock_next()
    { 
      int loop = 0;
      while (!try_lock_next()) { 
        hwthread_pause(); 
#warning remove counting for production
        loop++;
        PANIC_MSG(loop < 2," locking failed too many times");
      }
    }

    void unlock_next()
    { 
      MLOG_ERROR(mlog::cap, __PRETTY_FUNCTION__, DVAR(this));
      auto res = _next.fetch_and(~LOCKED_FLAG);
      ASSERT(res & LOCKED_FLAG);
    }

    /* deletion lock functions protect the deletion of a object  */

    bool try_lock_cap()
    { 
      bool ret = !(_prev.fetch_or(LOCKED_FLAG) & LOCKED_FLAG);
      MLOG_ERROR(mlog::cap, __PRETTY_FUNCTION__, DVAR(this), ret? " locked" : "locking failed!");
      return ret;
    }

    void lock_cap()
    { 
      int loop = 0;
      while (!try_lock_cap()) { 
        hwthread_pause(); 
#warning remove counting for production
        loop++;
        PANIC_MSG(loop < 2," locking failed too many times");
      }
    }

    void unlock_cap()
    { 
      MLOG_ERROR(mlog::cap, __PRETTY_FUNCTION__, DVAR(this));
      auto res = _prev.fetch_and(~LOCKED_FLAG);
      ASSERT(res & LOCKED_FLAG);
    }

    /// only for assertions and debugging
    /// only trust the result if it is false and it should be true
    bool next_is_locked() const { return _next.load() & CapEntry::LOCKED_FLAG; }

    /* lock prev functions protect the link to the next CapEntry */

    Error try_lock_prev();
    bool lock_prev();
    void unlock_prev()
    { 
      MLOG_ERROR(mlog::cap, __PRETTY_FUNCTION__, DVAR(this));
      Link(_prev)->unlock_next();
    }

    CapEntry* next()
    {
      ASSERT(next_is_locked());
      return Link(_next).ptr();
    }

    CapEntry* prev()
    {
      return Link(_prev).ptr();
    }

    bool isLinked() const { return cap().isAllocated() && Link(_next).ptr() && Link(_prev).ptr(); }
    bool isUnlinked() const { return cap().isZombie() && !Link(_next).ptr() && !Link(_prev).ptr(); }

  private:

    // called by move and insertAfter
    void setPrevPreserveFlags(CapEntry* ptr);

    // lock flag in _next and _prev
    // _next protects the link to the next entry (lock_next)
    // _prev protects the capability in the entry from being changed (lock_cap)
    static constexpr uintlink_t LOCKED_FLAG = 1;

    // flags describing the entry in _prev
    static constexpr uintlink_t REVOKING_FLAG = 1 << 1; // prevents from moving
    static constexpr uintlink_t DELETED_FLAG = 1 << 2; // prevents from inserting in soon-to-be-deleted object

    static constexpr uintlink_t FLAG_MASK = 7;

    static_assert((DELETED_FLAG | REVOKING_FLAG | FLAG_MASK) == FLAG_MASK, "prev flags do not fit");
    static_assert((LOCKED_FLAG | FLAG_MASK) == FLAG_MASK, "next flags do not fit");

    class Link {
    public:
      Link() : _val(0) {}

      explicit Link(uintlink_t value) : _val(value) {}
      Link(CapEntry* ptr, uintlink_t flags = 0) : _val(_pack(ptr, flags)) {}

      uintlink_t value() const { return _val; }
      CapEntry* operator->() { ASSERT(ptr()); return ptr(); }

      Link withFlags(uintlink_t flags) const { return Link(_offset(),  flags); }
      Link withoutFlags() const { return Link(_offset(), 0); }

      Link withPtr(CapEntry* ptr) const { return Link(ptr, flags()); }

      CapEntry* ptr() const
      {
        auto offset = _offset();
        // null-offset is nullptr
        return offset ? offset2kernel<CapEntry>(offset) : nullptr;
      }

      uintlink_t flags() const {
        return _val & FLAG_MASK;
      }

    protected:

      Link(uintlink_t offset, uintlink_t flags) : _val(_pack(offset, flags)) {}

      uintlink_t _offset() const {
        return _val & ~FLAG_MASK;
      }

      static uintlink_t _pack(uintlink_t offset, uintlink_t flags)
      {
        ASSERT((flags | FLAG_MASK) == FLAG_MASK);
        ASSERT_MSG((offset & FLAG_MASK) == 0, "unaligned CapRef)");
        return offset | flags;
      }

      static uintlink_t _pack(CapEntry* entry, uintlink_t flags)
      {
        // nullptr is null-offset
        auto offset = entry ? kernel2offset(entry) : 0u;
        return _pack(offset, flags);
      }

      const uintlink_t _val;
    };

  protected:
    std::atomic<uintcap_t> _cap;
    std::atomic<uintlink_t> _prev; // all flags that are independent from the locking go here
    std::atomic<uintlink_t> _next; // all flags that are set or reset with the locking go here
  };

  template<typename COMMITFUN>
  optional<void> CapEntry::insertAfter(Cap parentCap, 
                                       CapEntry& targetEntry, Cap targetCap, 
                                       COMMITFUN const& commit)
  {
    MLOG_ERROR(mlog::cap, __PRETTY_FUNCTION__, DVAR(this), DVAR(parentCap), DVAR(targetEntry), DVAR(targetCap));
    ASSERT(isKernelAddress(this));
    ASSERT(targetEntry.cap().isAllocated());
    lock_next(); // lock the parent entry, the child is already acquired
    auto curCap = cap();
    // lazy-locking: check that we still operate on the same parent capability
    if (!curCap.isUsable() || curCap != parentCap) {
      unlock_next(); // unlock the parent entry
      targetEntry.reset(); // release exclusive usage and revert to an empty entry
      THROW(Error::LOST_RACE);
    }

    // exec commit function while parent and child are still locked and the insert was successful
    commit(); 

    auto next = Link(_next.load()).withoutFlags();
    next->setPrevPreserveFlags(&targetEntry);
    MLOG_ERROR(mlog::cap, "this unlocks _next remotely ", DVAR(targetEntry));
    targetEntry._next.store(next.value());
    // deleted or revoking can not be set in target._prev
    // as we allocated target for being inserted
    MLOG_ERROR(mlog::cap, "this unlocks _prev remotely ", DVAR(targetEntry));
    targetEntry._prev.store(Link(this).value());
    MLOG_ERROR(mlog::cap, "this unlocks _next ");
    this->_next.store(Link(&targetEntry).value()); // unlocks the parent entry
    targetEntry.commit(targetCap); // release the target entry as usable
    RETURN(Error::SUCCESS);
  }

  
  template<class T>
  ostream_base<T>& operator<<(ostream_base<T>& out, const CapEntry& entry)
  {
    auto cap = entry.cap();
    out << cap;
    if (entry.isLinked()) out << ":linked";
    if (entry.isDeleted()) out << ":deleted";
    if (entry.isUnlinked()) out << ":unlinked";
    if (entry.next_is_locked()) out << ":next_locked";
    if (entry.isRevoking()) out << ":revoking";
    return out;
  }

} // namespace mythos
