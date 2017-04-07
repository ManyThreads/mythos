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

#include "runtime/CapMap.hh"
#include "runtime/Portal.hh"
#include "util/optional.hh"
#include <atomic>

namespace mythos {

  class SimpleCapAlloc
  {
  public:
    SimpleCapAlloc(uint32_t start, uint32_t count)
      : start(start), end(uint64_t(start)+count), mark(start) {}

    CapPtr alloc() {
      uint64_t o = mark;
      do {
        if (o>=end) return mythos::init::NULLCAP;
      } while (!mark.compare_exchange_weak(o, o+1));
      return CapPtr(o);
    }

    CapPtr operator() () { return alloc(); }

  protected:
    uint64_t start;
    uint64_t end;
    std::atomic<uint64_t> mark;
  };

  class SimpleCapAllocDel : public SimpleCapAlloc
  {
  public:
    SimpleCapAllocDel(Portal& portal, CapMap cs, uint32_t start, uint32_t count)
      : SimpleCapAlloc(start, count), portal(&portal), cs(cs) {}

    optional<void> free(KObject p, PortalLock& pl) { return free(p.cap(), pl); }
    optional<void> free(CapPtr p, PortalLock& pl) {
      ASSERT(start <= p && p < mark);
      return optional<void>(cs.deleteCap(pl, p).wait().state());
    }
    void freeAll(PortalLock& pl) {
      for (uint32_t i=start; i < mark; i++) {
        cs.deleteCap(pl, CapPtr(i)).wait();
      }
      mark = start;
    }

    optional<void> free(KObject p) { return free(p.cap()); }
    optional<void> free(CapPtr p) {
      mythos::PortalLock pl(*portal);
      return free(p, pl);
    }
    void freeAll() {
      mythos::PortalLock pl(*portal);
      freeAll(pl);
    }

  protected:
    Portal* portal;
    CapMap cs;
  };

} // namespace mythos
