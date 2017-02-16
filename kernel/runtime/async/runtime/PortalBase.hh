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

#include "runtime/FutureBase.hh"
#include "runtime/ISysretHandler.hh"
#include "mythos/InvocationBuf.hh"
#include "mythos/protocol/Portal.hh"
#include <atomic>
#include <utility>

namespace mythos {

  class KObject
  {
  public:
    KObject(CapPtr cap) : _cap(cap) {}
    CapPtr cap() const { return _cap; }
  protected:
    CapPtr _cap;
  };

  class PortalBase
    : public KObject, protected ISysretHandler, public FutureBase
  {
  public:
    PortalBase(CapPtr cap, void* ib) : KObject(cap), _buf(reinterpret_cast<InvocationBuf*>(ib)) {}
    PortalBase(PortalBase const&) = delete;
    virtual ~PortalBase() { OOPS(refcount==0); }

    InvocationBuf* buf() const { return _buf; }

    bool acquire() { int exp = 0; return refcount.compare_exchange_strong(exp, 1); }
    void incref() { refcount.fetch_add(1); }
    void release() { refcount.fetch_sub(1); }

    void invoke(CapPtr kobj) {
      FutureBase::reset();
      ISysretHandler::handle(syscall_invoke_poll(_cap, kobj, (ISysretHandler*)this));
    }

  protected:
    void sysret(uint64_t e) override { FutureBase::assign(Error(e)); }

  protected:
    InvocationBuf* _buf;
    std::atomic<int> refcount = {0};
  };

  class PortalRef
  {
  public:
    PortalRef() {}
    PortalRef(PortalBase& p) { if (p.acquire()) _portal = &p; }
    PortalRef(PortalRef const& p) : _portal(p._portal) { if (_portal) _portal->incref(); }
    PortalRef(PortalRef&& p) : _portal(p._portal) { p._portal = nullptr; }
    ~PortalRef() { close(); }

    PortalRef& operator= (PortalRef&& p) {
      close();
      _portal = p._portal;
      p._portal = nullptr;
      return *this;
    }

    void close() { if (_portal) _portal->release(); _portal = nullptr; }
    bool isOpen() const { return _portal; }

    template<class MSG, class... ARGS>
    MSG* write(ARGS const&... args) { ASSERT(_portal); return _portal->buf()->write<MSG>(args...); }

    void invoke(CapPtr kobj) { ASSERT(_portal); _portal->invoke(kobj); }

    template<class MSG, class... ARGS>
    PortalRef tryInvoke(CapPtr kobj, ARGS const&... args) {
      if (_portal) {
        _portal->buf()->write<MSG>(args...);
        _portal->invoke(kobj);
      }
      return *this;
    }

  protected:
    PortalBase* _portal = nullptr;
  };

  class PortalFutureRefBase
    : protected PortalRef
  {
  public:
    PortalFutureRefBase(PortalRef&& p) : PortalRef(std::move(p)) {}
    void close() { PortalRef::close(); }
    PortalRef&& reuse() { return std::move(*this); }

    bool operator! () const { return !isOpen(); }
    explicit operator bool() const { return isOpen(); }
    bool poll() const { return isOpen() ? _portal->poll() : true; }
    bool wait() const { return isOpen() ? _portal->wait() : true; }
    Error state() const { return !isOpen() ? Error::PORTAL_NOT_OPEN : _portal->error(); }

    void then(Tasklet& t) { ASSERT(isOpen()); _portal->then(t); close(); }
  };

  template<class T>
  class PortalFutureRef
    : public PortalFutureRefBase
  {
  public:
    PortalFutureRef(PortalRef&& p) : PortalFutureRefBase(std::move(p)) {}

    /** quick access to the invocation result */
    T const* operator-> () const { return _portal->buf()->cast<T>(); }
    T const& operator* () const { return *_portal->buf()->cast<T>(); }
  };

  template<>
  class PortalFutureRef<void>
    : public PortalFutureRefBase
  {
  public:
    PortalFutureRef(PortalRef&& p) : PortalFutureRefBase(std::move(p)) {}
  };

} // namespace mythos
