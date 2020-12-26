/* -*- mode:C++; indent-tabs-mode:nil; -*- */
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
 * Copyright 2020 Philipp Gypser and contributors, BTU Cottbus-Senftenberg
 */
#pragma once

#include "runtime/CapMap.hh"
#include "runtime/Portal.hh"
#include "util/optional.hh"
#include "runtime/Mutex.hh"

namespace mythos {

  template<uint32_t START, uint32_t COUNT>
  class SimpleCapAlloc
  {
  public:
    SimpleCapAlloc(Portal& portal, CapMap cs)
      : next(START) 
      , top(0)
      , portal(&portal)
      , cs(cs)
    {}

    CapPtr alloc() {
      Mutex::Lock guard(m);

      // try taking cap from stack
      if(top){
        top--;
        MLOG_DETAIL(mlog::app, __func__, "return cap from stack", caps[top]);
        return caps[top];
      }
      // else try taking cap from range
      else if(next < START + COUNT){
        ASSERT(next >= START);
        auto ret = next;
        next++;
        MLOG_DETAIL(mlog::app, __func__, "return cap from range", ret);
        return ret;
      }
      // fail: no free cap
      else{
        MLOG_ERROR(mlog::app, __func__, "Out of Caps!");
        return null_cap;
      }

    }

    CapPtr operator() () { return alloc(); }

    void freeEmpty(CapPtr p){
      MLOG_DETAIL(mlog::app, __func__, DVAR(p));
      Mutex::Lock guard(m);
      ASSERT(top < COUNT);
      caps[top] = p;
      top++;
    }

    optional<void> free(CapPtr p, PortalLock& pl) {
      MLOG_DETAIL(mlog::app, __func__, DVAR(p));
      ASSERT(START <= p && p < START + COUNT);
      auto res = cs.deleteCap(pl, p).wait();
      if(res) freeEmpty(p);
      else MLOG_ERROR(mlog::app, __func__, "deleteCap failed!");
      return res;
    }

    optional<void> free(KObject p, PortalLock& pl) { return free(p.cap(), pl); }

  protected:
    uint32_t next;
    uint32_t top;
    Portal* portal;
    CapMap cs;
    CapPtr caps[COUNT];
    Mutex m; 
  };

} // namespace mythos
