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

#include "util/error-trace.hh"
#include "objects/Cap.hh"
#include "objects/IKernelObject.hh"

namespace mythos {

  class CapEntry;
  class IInvocation;
  class IAllocator;
  class IInvocation;

  class IFactory
  {
  public:
    virtual ~IFactory() {}

    /** abstract factory method for all kernel objects that can create
     * other kernel objects.  The asynchronous invocation goes to the
     * source untyped memory and the untyped memory then calls this
     * method synchronously. The IAllocator is provided by the untyped
     * memory and is used synchronously because the initial
     * asynchronous invocation already protects untyped memory
     * object. In order to ensure parallel scalability, the factory
     * method can be called concurrently.
     *
     * @return returns an error code or Error::SUCCESS.
     */
    virtual Error factory(CapEntry* dstEntry, CapEntry* memEntry, Cap memCap, IAllocator* mem,
                          IInvocation* data) const = 0;
  };

  class FactoryBase
    : public IKernelObject, public IFactory
  {
  public:
    optional<void const*> vcast(TypeId id) const override {
      if (id == typeId<IFactory>()) return static_cast<IFactory const*>(this);
      THROW(Error::TYPE_MISMATCH);
    }

    optional<void> deleteCap(Cap, IDeleter&) override { RETURN(Error::SUCCESS); }
  };

} // namespace mythos
