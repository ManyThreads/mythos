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

namespace mythos {

  class SimpleCapAlloc
  {
  public:
    SimpleCapAlloc(Portal* portal, CapMap cm, uint32_t start, uint32_t count)
      : portal(portal), cm(cm), start(start), end(start+count), mark(start) {}

    optional<CapPtr> alloc() {
      if (mark >= end) return Error::INSUFFICIENT_RESOURCES;
      mark++;
      return CapPtr(mark-1);
    }

    void freeObject(KObject p) { freeObject(p.cap()); }

    void freeObject(CapPtr p) {
      ASSERT(start <= p && p < mark);
      auto res = cm.deleteCap(*portal, p);
      res.wait();
      ASSERT(res);
      freePtr(p);
      res.close();
    }

    void freePtr(CapPtr) {}

    void freeAllObjects() {
      for (uint32_t i=start; i < mark; i++) {
        auto res = cm.deleteCap(*portal, CapPtr(i));
        res.wait();
        freePtr(CapPtr(i));
        res.close();
      }
      mark = start;
    }

  protected:
    Portal* portal;
    CapMap cm;
    uint32_t start;
    uint32_t end;
    uint32_t mark;
  };

} // namespace mythos
