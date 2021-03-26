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

#include "objects/SignalListener.hh"
#include <new>
#include "mythos/InvocationBuf.hh"
#include "util/assert.hh"
#include "util/PhysPtr.hh"
#include "objects/ops.hh"
#include "objects/KernelMemory.hh"
#include "objects/TypedCap.hh"
#include "boot/memory-root.hh"
#include "objects/DebugMessage.hh"
#include "util/error-trace.hh"

namespace mythos {

  optional<void const*> SignalListener::vcast(TypeId id) const
  {
    if (id == typeId<SignalListener>()) return /*static_cast<ExampleObj const*>*/(this);
    THROW(Error::TYPE_MISMATCH);
  }

  optional<void> SignalListener::deleteCap(CapEntry&, Cap self, IDeleter& del)
  {
    if (self.isOriginal()) {
      del.deleteObject(del_handle);
    }
    RETURN(Error::SUCCESS);
  }

  void SignalListener::deleteObject(Tasklet* t, IResult<void>* r)
  {
    monitor.doDelete(t, [=](Tasklet* t){
      _mem->free(t, r, this, sizeof(ExampleObj));
    });
  }

  void SignalListener::invoke(Tasklet* t, Cap self, IInvocation* msg)
  {
    monitor.request(t, [=](Tasklet* t){
        Error err = Error::NOT_IMPLEMENTED;
        switch (msg->getProtocol()) {
        case protocol::KernelObject::proto:
          err = protocol::KernelObject::dispatchRequest(this, msg->getMethod(), self, msg);
          break;
        case protocol::SignalListener::proto:
          err = protocol::SignalListener::dispatchRequest(this, msg->getMethod(), t, self, msg);
          break;
        }
        if (err != Error::INHIBIT) {
          msg->replyResponse(err);
          monitor.requestDone();
        }
      } );
  }

  Error SignalListener::invokeBind(Tasklet*, Cap, IInvocation* msg)
  {
    auto data = msg->getMessage()->read<protocol::SignalListener::Bind>();

    // TODO set mask
    // TODO context

    if (data.signalSource() == delete_cap) { _source.reset(); }
    else if (data.signalSource() != null_cap) {
      auto err = setSource(msg->lookupEntry(data.signalSource()));
      if (!err) return err.state();
    }
    if (data.keventSink() == delete_cap) { _sink.reset(); }
    else if (data.keventSink() != null_cap) {
      auto err = setSink(msg->lookupEntry(data.keventSink()));
      if (!err) return err.state();
    }
    return Error::SUCCESS;
  }

  optional<void> SignalListener::setSource(optional<CapEntry*> entry)
  {
    TypedCap<ISignalSource> src(entry);
    if (!src) RETHROW(src);
    RETURN(_source.set(this, *entry, src.cap()));
  }

  void SignalListener::bind(optional<ISignalSource*>)
  {
    // TODO
  }

  void SignalListener::unbind(optional<ISignalSource*>)
  {
    // TODO
  }

  optional<void> SignalListener::setSink(optional<CapEntry*> entry)
  {
    TypedCap<IKEventSink> sink(entry);
    if (!sink) RETHROW(sink);
    RETURN(_sink.set(this, *entry, sink.cap()));
  }

  void SignalListener::bind(optional<IKEventSink*>)
  {
    // TODO
  }

  void SignalListener::unbind(optional<IKEventSink*>)
  {
    // TODO
  }

  KEvent SignalListener::deliverKEvent()
  {
    // TODO
    return KEvent(0, uint64_t(Error::NO_MESSAGE));
  }

  void SignalListener::signalChanged(Signal newValue)
  {
    // TODO
  }

  Error SignalListener::getDebugInfo(Cap self, IInvocation* msg)
  {
    return writeDebugInfo("SignalListener", self, msg);
  }

  optional<SignalListener*>
  SignalListenerFactory::factory(CapEntry* dstEntry, CapEntry* memEntry, Cap memCap,
                         IAllocator* mem)
  {
    auto obj = mem->create<SignalListener>();
    if (!obj) {
      dstEntry->reset();
      RETHROW(obj);
    }
    Cap cap(*obj);
    auto res = cap::inherit(*memEntry, memCap, *dstEntry, cap);
    if (!res) {
      mem->free(*obj); // mem->release(obj) goes throug IKernelObject deletion mechanism
      RETHROW(res);
    }
    return *obj;
  }

} // mythos
