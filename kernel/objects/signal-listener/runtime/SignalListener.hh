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

#include "runtime/PortalBase.hh"
#include "mythos/protocol/SignalListener.hh"
#include "runtime/KernelMemory.hh"
#include "mythos/init.hh"

namespace mythos {

  class SignalListener : public KObject, public ISysretHandler, public FutureBase
  {
  public:

    typedef protocol::SignalListener::Context Context;
    typedef protocol::SignalListener::Signal Signal;

    SignalListener() {}
    SignalListener(CapPtr cap) : KObject(cap) {}

    PortalFuture<void> create(PortalLock pr, KernelMemory kmem,
                              CapPtr factory = init::SIGNAL_LISTENER_FACTORY) {
      return pr.invoke<protocol::SignalListener::Create>(kmem.cap(), _cap, factory);
    }

   PortalFuture<void> bind(
            PortalLock pr,
            CapPtr signalSource, Signal mask, CapPtr keventSink,
            Signal resetMask = protocol::SignalListener::RESET_ALL)
   {
     ISysretHandler* handler = this;
     auto context = Context(handler);
      return pr.invoke<protocol::SignalListener::Bind>(_cap, signalSource, mask, context, keventSink, resetMask);
   }

   PortalFuture<void> reset( PortalLock pr, Signal resetMask = protocol::SignalListener::RESET_ALL)
   {
      return pr.invoke<protocol::SignalListener::Reset>(_cap, resetMask);
   }

   // ISysRetHandler
   void sysret(uint64_t signal) override
   {
     _signal |= signal;
     FutureBase::assign(Error::SUCCESS);
   }

   Signal peekSignal() { return _signal; }

   Signal resetSignal()
   {
     auto res = _signal;
     _signal = 0;
     return res;
   }

  protected:
   
   Signal _signal = 0;

  };

} // namespace mythos
