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
      _mem->free(t, r, this, sizeof(SignalListener));
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


    if (data.signalSource() == delete_cap) {
      _mutex << [this] { _mask = 0; };
      _source.reset();
    } else if (data.signalSource() != null_cap) {
      _mutex << [this] { _mask = 0; };
      auto err = setSource(msg->lookupEntry(data.signalSource()));
      if (!err) return err.state();
    }
    _mutex << [this, &data] {
      _mask = data.mask;
      _context = data.context;
      _autoResetMask = data.autoResetMask;
    };

    if (data.keventSink() == delete_cap) { _sink.reset(); }
    else if (data.keventSink() != null_cap) {
      auto err = setSink(msg->lookupEntry(data.keventSink()));
      if (!err) return err.state();

    }
    _mutex << [this, &data] {
      _signal &= ~data.resetMask;
      // Comment on races after bind of kevent sink:
      // read cap fresh from _sink entry because of race with unbind after bind
      // if it was called, the cap is killed now (-> not usable)
      // we can not race with another binding call,
      // because this is protected by the monitor
      TypedCap<IKEventSink> sink(_sink);
      if (!sink) return;
      if (_signal && !_eventAttached) {
        sink->attachKEvent(&_eventHandle);
        _eventAttached = true;
      } else if (!_signal && _eventAttached) {
        sink->detachKEvent(&_eventHandle);
        _eventAttached = false;
      }
    };

    return Error::SUCCESS;
  }

  Error SignalListener::invokeReset(Tasklet*, Cap, IInvocation* msg)
  {
    auto data = msg->getMessage()->read<protocol::SignalListener::Reset>();
    _mutex << [this, &data] {
      _signal &= ~data.resetMask;
      TypedCap<IKEventSink> sink(_sink);
      if (!sink) return;
      if (_signal && !_eventAttached) {
        sink->attachKEvent(&_eventHandle);
        _eventAttached = true;
      } else if (!_signal && _eventAttached) {
        sink->detachKEvent(&_eventHandle);
        _eventAttached = false;
      }
    };
    return Error::SUCCESS;
  }


  optional<void> SignalListener::setSource(optional<CapEntry*> entry)
  {
    TypedCap<ISignalSource> src(entry);
    if (!src) RETHROW(src);
    RETURN(_source.set(this, *entry, src.cap()));
  }

  void SignalListener::bind(optional<ISignalSource*> source)
  {
    ASSERT(source);
    source->attachSignalSink(&_listenerHandle);
  }

  void SignalListener::unbind(optional<ISignalSource*> source)
  {
    ASSERT(source);
    source->detachSignalSink(&_listenerHandle);
  }

  optional<void> SignalListener::setSink(optional<CapEntry*> entry)
  {
    TypedCap<IKEventSink> sink(entry);
    if (!sink) RETHROW(sink);
    RETURN(_sink.set(this, *entry, sink.cap()));
  }

  void SignalListener::bind(optional<IKEventSink*> sink)
  {
    ASSERT(sink);
    // this function is called BEFORE the cap is commited into the CapRef in CapEntry::insertAfter
    // we can not attach to _sink here because of races with signal changed
  }

  void SignalListener::unbind(optional<IKEventSink*> sink)
  {
    // this function is called AFTER the cap is killed in resetReference()
    // and before it is reset
    ASSERT(sink);
    _mutex << [this, sink] {
      sink->detachKEvent(&_eventHandle);
      _eventAttached = false;
    };
  }

  KEvent SignalListener::deliverKEvent()
  {
    KEvent result;
    _mutex << [this, &result] {
      result.user = _context;
      result.state = _signal;

      _eventAttached = false;
      _signal &= ~_autoResetMask;

      if (_signal) {
        // reattach event
        TypedCap<IKEventSink> sink(_sink);
        if (!sink) return;
        sink->attachKEvent(&_eventHandle);
        _eventAttached = true;
      }
    };
    return  result;
  }

  void SignalListener::signalChanged(Signal newValue)
  {
    _mutex << [this, newValue] {
      _signal |= newValue & _mask;
      if (_signal) {
        // problem: _sink might be currently bound, but not commited yet
        // solution: invokation must recheck after binding 
        TypedCap<IKEventSink> sink(_sink);
        if (sink && !_eventAttached) {
          sink->attachKEvent(&_eventHandle);
          _eventAttached = true;
        }
      }
    };
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
