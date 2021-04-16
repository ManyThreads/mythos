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
#include "util/ThreadMutex.hh"

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
  Error invokeReset(Tasklet* t, Cap self, IInvocation* msg);

  // Resets the bits in signal that are set in reset mask.
  // Detaches from / attaches to the event queue when nessecary,
  // even when no bits are reset by resetWithMask itself.
  void resetWithMask(Signal resetMask);

protected:

  friend class CapRefBind;


  CapRef<SignalListener, ISignalSource> _source;
  // handle is in source list <-> listener is bound to source
  // the handle is protected by the CapRef lock
  ISignalSource::handle_t _listenerHandle = {this};

  optional<void> setSource(optional<CapEntry*> entry);
  void bind(optional<ISignalSource*>);
  void unbind(optional<ISignalSource*>);


  CapRef<SignalListener, IKEventSink> _sink;
  // handle is in sink list <-> listener is bound to sink && signal != 0
  // protected by CapRef lock AND _mutex
  // lock order: CapRef lock first, then _mutex
  IKEventSink::handle_t _eventHandle = {this};
  optional<void> setSink(optional<CapEntry*> entry);
  void bind(optional<IKEventSink*>);
  void unbind(optional<IKEventSink*>);

  // mutex to protect signal, mask, context, _eventAttached and _eventHandle
  // there are some complex relationships between these member
  // and its better if we update them atomically for now
  // _eventAttached serves a similar purpose as the `state` member in Portal
  ThreadMutex _mutex;
  bool _eventAttached = false;
  Signal _mask = 0;
  Signal _signal = 0;
  KEvent::Context _context = 0;

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
