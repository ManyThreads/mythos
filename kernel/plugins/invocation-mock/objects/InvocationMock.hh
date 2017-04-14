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
 * Copyright 2016 Randolf Rotta, Robert Kuban, and contributors, BTU Cottbus-Senftenberg
 */
#pragma once

#include "objects/IExecutionContext.hh"
#include "objects/IInvocation.hh"

namespace mythos {

  class InvocationMock : public IInvocation
  {
  public:

    InvocationMock(CapEntry& entry) : _entry(&entry), _cap(entry.cap()) {}
    InvocationMock(CapEntry& entry, Cap cap) : _entry(&entry), _cap(cap) {}

    virtual void replyResponse(optional<void>) override { ASSERT(!"NOT IMPLEMENTED"); }
    virtual void enqueueResponse(LinkedList<IInvocation*>&, IPortal*) override { ASSERT(!"NOT IMPLEMENTED"); }
    virtual void deletionResponse(CapEntry*, bool) override { ASSERT(!"NOT IMPLEMENTED"); }

    virtual Cap getCap() const override { return _cap; }
    virtual CapEntry* getCapEntry() const override { return _entry; }

    virtual IExecutionContext* getEC() override { ASSERT(!"NOT IMPLEMENTED"); }
    virtual InvocationBuf* getMessage()  override { return nullptr; }

    virtual size_t getMaxSize() override { return 0; }

    IInvocation* addr() { return this; };

  protected:
    CapEntry* _entry;
    Cap _cap;
  };

} // namespace mythos
