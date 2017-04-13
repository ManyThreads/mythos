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

#include "util/assert.hh"
#include "objects/CapEntry.hh"
#include "objects/ops.hh"

namespace mythos {

  /** The global root of the resource tree */
  class MemoryRoot final
    : public IKernelObject
  {
  public:
    void init() { rootEntry.initRoot(Cap(this)); }

    /** used to attach the static memory regions as children. */
    CapEntry& getRoot() { return rootEntry; } 

  public: // IKernelObject interface
    Range<uintptr_t> addressRange(Cap) override  { return {0, ~uintptr_t(0)}; }
    optional<void const*> vcast(TypeId) const override { THROW(Error::TYPE_MISMATCH); }
    optional<void> deleteCap(Cap, IDeleter&) override { PANIC(false); }
    void invoke(Tasklet*, Cap, IInvocation* msg) override {
      OOPS_MSG(false, "Invocation to root object.");
      msg->replyResponse(Error::INVALID_REQUEST);
    }

  protected:
    CapEntry rootEntry;
  };

} // namespace mythos
