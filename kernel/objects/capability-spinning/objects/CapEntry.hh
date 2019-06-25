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

    optional<void> insertAfter(const Cap& thisCap, CapEntry& target) WARN_UNUSED;
    // optional<void> insertBefore(const Cap& thisCap, CapEntry& target) WARN_UNUSED;
    optional<void> moveTo(CapEntry& other);

    bool setRevoking() { return !(_prev.fetch_or(REVOKING_FLAG) & REVOKING_FLAG); }
    void finishRevoke() { _prev.fetch_and(~REVOKING_FLAG); }
    bool isRevoking() const { return _prev.load() & REVOKING_FLAG; }

    void setDeleted() { _prev.fetch_or(DELETED_FLAG); }
    bool isDeleted() const { return _prev.load() & DELETED_FLAG; }

    optional<void> acquire();
    void commit(const Cap& cap);
    void reset();

    /** Bring entry into zombi state. Retries until zombified, empty or
     * allocated. Returns true if zombified. */ 
    bool kill();

    optional<void> unlink();
    bool try_lock() { return !(_next.fetch_or(LOCKED_FLAG) & LOCKED_FLAG); }
    void lock() { while (!try_lock()) { hwthread_pause(); } }
    void unlock() { auto res = _next.fetch_and(~LOCKED_FLAG); ASSERT(res & LOCKED_FLAG); }

    /// only for assertions and debugging
    /// only trust the result if it is false and it should be true
    bool is_locked() const { return _next.load() & CapEntry::LOCKED_FLAG; }

    Error try_lock_prev();
    bool lock_prev();
    void unlock_prev() { Link(_prev)->unlock(); }

    CapEntry* next()
    {
      ASSERT(is_locked());
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

    static constexpr uintlink_t LOCKED_FLAG = 1;
    static constexpr uintlink_t REVOKING_FLAG = 1 << 1;
    static constexpr uintlink_t DELETED_FLAG = 1 << 2;
    static constexpr uintlink_t FLAG_MASK = 7;

    static_assert((DELETED_FLAG | REVOKING_FLAG | FLAG_MASK) == FLAG_MASK, "prev flags do not fit");
    static_assert((LOCKED_FLAG | FLAG_MASK) == FLAG_MASK, "next flags do not fit");

    class Link {
    public:
      Link() : _val(0) {}
      Link(CapEntry* ptr, uintlink_t flags = 0) : _val(_pack(ptr, flags)) {}
      Link(uintlink_t value) : _val(value) {}
      operator uintlink_t() const { return val(); }
      CapEntry* operator->() { ASSERT(ptr()); return ptr(); }
      uintlink_t val() const { return _val; }
      CapEntry* ptr() const { return _toPtr(); }
      bool has(uintlink_t flags) const { return (_val & flags) == flags; }
      Link with(uintlink_t flags) const { return Link(_val & flags); }
      Link withFlags(const Link& other) const { return Link(_val).with(other._val & FLAG_MASK); }
    protected:
      static uintlink_t _pack(CapEntry* entry, uintlink_t flags)
      {
        ASSERT((kernel2phys(entry) & 7) == 0);
        return (entry ? kernel2phys(entry) : 0) | flags;
      }
      CapEntry* _toPtr() const { return _val ? phys2kernel<CapEntry>(_val & uintlink_t(~7)) : nullptr; }
      const uintlink_t _val;
    };

  protected:
    std::atomic<uintcap_t> _cap;
    std::atomic<uintlink_t> _prev; // all flags that are independent from the locking go here
    std::atomic<uintlink_t> _next; // all flags that are set or reset with the locking go here
  };

  template<class T>
  ostream_base<T>& operator<<(ostream_base<T>& out, const CapEntry& entry)
  {
    auto cap = entry.cap();
    out << cap;
    if (entry.isLinked()) out << ":linked";
    if (entry.isDeleted()) out << ":deleted";
    if (entry.isUnlinked()) out << ":unlinked";
    if (entry.is_locked()) out << ":locked";
    if (entry.isRevoking()) out << ":revoking";
    return out;
  }

} // namespace mythos
