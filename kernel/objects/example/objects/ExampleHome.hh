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

#include <array>
#include <cstddef>
#include "async/SimpleMonitorHome.hh"
#include "objects/IFactory.hh"
#include "objects/IKernelObject.hh"
#include "objects/CapEntry.hh"
#include "mythos/protocol/KernelObject.hh"
#include "mythos/protocol/Example.hh"

namespace mythos {

/**
 * Wastes space and does nothing.
 * - Serves as a example/template for the implementation of actual kernel objects
 * - Simple object for testing basic inheritance tree functionality.
 */
class ExampleHomeObj
  : public IKernelObject
{
public:
  ExampleHomeObj(IAsyncFree* mem) : _mem(mem) {}
  ExampleHomeObj(const ExampleHomeObj&) = delete;

  optional<void const*> vcast(TypeId id) const override;
  optional<void> deleteCap(Cap self, IDeleter& del) override;
  void deleteObject(Tasklet* t, IResult<void>* r) override;
  void invoke(Tasklet* t, Cap self, IInvocation* msg) override;

protected:
  friend struct protocol::KernelObject;
  Error getDebugInfo(Cap self, IInvocation* msg);

  friend struct protocol::Example;
  Error printMessage(Tasklet* t, Cap self, IInvocation* msg);
  Error ping(Tasklet* t, Cap self, IInvocation* msg);
  Error moveHome(Tasklet* t, Cap self, IInvocation* msg);

protected:
  async::SimpleMonitorHome monitor;
  IDeleter::handle_t del_handle = {this};
  IAsyncFree* _mem;
  friend class ExampleFactory;
};

class ExampleHomeFactory : public FactoryBase
{
public:
  static optional<ExampleHomeObj*>
  factory(CapEntry* dstEntry, CapEntry* memEntry, Cap memCap, IAllocator* mem);

  Error factory(CapEntry* dstEntry, CapEntry* memEntry, Cap memCap,
                IAllocator* mem, IInvocation*) const override {
    return factory(dstEntry, memEntry, memCap, mem).state();
  }
};

}  // namespace mythos
