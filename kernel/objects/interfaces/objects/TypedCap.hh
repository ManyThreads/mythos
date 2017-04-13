/* -*- mode:C++; indent-tabs-mode:nil; -*- */
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
#pragma once

#include "objects/IKernelObject.hh"
#include "objects/CapEntry.hh"
#include "objects/CapRef.hh"
#include "objects/IFrame.hh"
#include "util/optional.hh"
#include "util/assert.hh"

namespace mythos {

  template<class T>
  class TypedCap {
  public:
    typedef T value_t;
    TypedCap() {}
    TypedCap(optional<CapEntry*> const& e) { set(e); }
    TypedCap(optional<CapEntryRef> const& e) { set(e); }
    TypedCap(CapEntry* e) { set(e); }
    TypedCap(CapEntry& e) { set(e.cap()); }
    TypedCap(Cap c) { set(c); }
    template<class S, class R>
    TypedCap(CapRef<S,T,R> const& e) { set(e.cap()); }

    operator Error() const { return _state; }
    operator optional<void>() const { return optional<void>(_state); }
    Error state() const { return _state; }
    bool operator!() const { return _state != Error::SUCCESS; }
    operator bool() const { return _state == Error::SUCCESS; }

    Cap cap() const { ASSERT(*this); return _cap; }
    T* obj() const { ASSERT(*this); return _obj; }
    IKernelObject* ptr() const { ASSERT(*this); return _cap.getPtr(); }
    T* operator->() const { return obj(); }
    T* operator*() const { return obj(); }

    TypedCap& set(optional<CapEntry*> const& e);
    TypedCap& set(optional<CapEntryRef> const& e);
    TypedCap& set(CapEntry* e);
    TypedCap& set(Cap c);

  public: // IKernelObject interface
    Range<uintptr_t> addressRange() const { return ptr()->addressRange(cap()); }
    optional<Cap> mint(CapRequest request, bool derive) const { return ptr()->mint(_cap, request, derive); }
    optional<void> deleteCap(IDeleter& del) const { return ptr()->deleteCap(_cap, del); }
    void invoke(Tasklet* t, IInvocation* msg) const { return ptr()->invoke(t, _cap, msg); }

  public: // ICapMap interface
    optional<CapEntryRef> lookup(CapPtr needle, CapPtrDepth maxDepth, bool writeable) const
    { return obj()->lookup(_cap, needle, maxDepth, writeable); }

  public: // IPortal interface
    optional<void> sendInvocation(CapPtr dest, uint64_t user) const { return obj()->sendInvocation(_cap, dest, user); }

  public: // IFrame interface
    IFrame::Info getFrameInfo() const { return obj()->getFrameInfo(_cap); }

  private:
    Cap _cap;
    T* _obj = nullptr;
    Error _state = Error::UNSET;
  };

  template<class T>
  TypedCap<T>& TypedCap<T>::set(optional<CapEntry*> const& e)
  {
    if (!e) { _state = e.state(); return *this; }
    return set(e->cap());
  }

  template<class T>
  TypedCap<T>& TypedCap<T>::set(CapEntry* e)
  {
    if (!e) { _state = Error::INVALID_CAPABILITY; return *this; }
    return set(e->cap());
  }

  template<class T>
  TypedCap<T>& TypedCap<T>::set(optional<CapEntryRef> const& e)
  {
    if (!e) { _state = e.state(); return *this; }
    return set(e->entry->cap());
  }

  template<class T>
  TypedCap<T>& TypedCap<T>::set(Cap c)
  {
    if (!c.isUsable()) { _state = Error::INVALID_CAPABILITY; return *this; }
    auto ptr = c.getPtr()->template cast<T>();
    if (!ptr) { _state = ptr.state(); return *this; }
    _cap = c;
    _obj = *ptr;
    _state = Error::SUCCESS;
    return *this;
  }

} // namespace mythos;
