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
#include "async/NestedMonitorDelegating.hh"
#include "objects/IFactory.hh"
#include "objects/IKernelObject.hh"
#include "objects/CapEntry.hh"
#include "objects/signal.hh"
#include "objects/kevent.hh"
#include "objects/CapRef.hh"
#include "mythos/protocol/KernelObject.hh"
#include "mythos/protocol/SignalListener.hh"

namespace mythos {

class SignalListener final
  : public IKernelObject
  , public ISignalSink
  , public IKEventSource
{
public:
  SignalListener(IAsyncFree* mem) : _mem(mem) {}
  SignalListener(const SignalListener&) = delete;

  optional<void const*> vcast(TypeId id) const override;
  optional<void> deleteCap(CapEntry&, Cap self, IDeleter& del) override;
  void deleteObject(Tasklet* t, IResult<void>* r) override;
  void invoke(Tasklet* t, Cap self, IInvocation* msg) override;

  KEvent deliverKEvent() override;
  void signalChanged(Signal newValue) override;


protected:
  friend struct protocol::KernelObject;
  Error getDebugInfo(Cap self, IInvocation* msg);

  friend struct protocol::SignalListener;
  Error invokeBind(Tasklet* t, Cap self, IInvocation* msg);

protected:

  friend class CapRefBind;

  CapRef<SignalListener, ISignalSource> _source;
  optional<void> setSource(optional<CapEntry*> entry);
  void bind(optional<ISignalSource*>);
  void unbind(optional<ISignalSource*>);

  CapRef<SignalListener, IKEventSink> _sink;
  optional<void> setSink(optional<CapEntry*> entry);
  void bind(optional<IKEventSink*>);
  void unbind(optional<IKEventSink*>);

  std::atomic<Signal> mask;
  std::atomic<Signal> signal;
  std::atomic<KEvent::Context> context;

  async::NestedMonitorDelegating monitor;
  IDeleter::handle_t del_handle = {this};
  IAsyncFree* _mem;
  friend class SignalListenerFactory;
};

class SignalListenerFactory : public FactoryBase
{
public:
  static optional<SignalListener*>
  factory(CapEntry* dstEntry, CapEntry* memEntry, Cap memCap, IAllocator* mem);

  Error factory(CapEntry* dstEntry, CapEntry* memEntry, Cap memCap,
                IAllocator* mem, IInvocation*) const override {
    return factory(dstEntry, memEntry, memCap, mem).state();
  }
};

}  // namespace mythos
