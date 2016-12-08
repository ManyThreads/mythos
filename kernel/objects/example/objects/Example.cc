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

#include <new>
#include "mythos/InvocationBuf.hh"
#include "util/assert.hh"
#include "util/PhysPtr.hh"
#include "util/alignments.hh"
#include "objects/Example.hh"
#include "objects/ops.hh"
#include "objects/UntypedMemory.hh"
#include "boot/memory-root.hh"
#include "objects/DebugMessage.hh"

namespace mythos {

  static mlog::Logger<mlog::FilterAny> mlogex("ExampleObj");

  optional<void const*> ExampleObj::vcast(TypeId id) const
  {
    mlogex.info("vcast", DVAR(this), DVAR(id.debug()));
    if (id == TypeId::id<ExampleObj>()) return /*static_cast<ExampleObj const*>*/(this);
    return Error::TYPE_MISMATCH;
  }

  optional<void> ExampleObj::deleteCap(Cap self, IDeleter& del)
  {
    mlogex.info("deleteCap", DVAR(this), DVAR(self), DVAR(self.isOriginal()));
    if (self.isOriginal()) {
      del.deleteObject(del_handle);
    }
    return Error::SUCCESS;
  }

  void ExampleObj::deleteObject(Tasklet* t, IResult<void>* r)
  {
    mlogex.info("deleteObject", DVAR(this), DVAR(t), DVAR(r));
    monitor.doDelete(t, [=](Tasklet* t){
      _mem->free(t, r, this, sizeof(ExampleObj));
    });
  }

  void ExampleObj::invoke(Tasklet* t, Cap self, IInvocation* msg)
  {
    monitor.request(t, [=](Tasklet* t){
        Error err = Error::NOT_IMPLEMENTED;
        switch (msg->getProtocol()) {
        case protocol::KernelObject::proto:
          err = protocol::KernelObject::dispatchRequest(this, msg->getMethod(), self, msg);
          break;
        case protocol::Example::proto:
          err = protocol::Example::dispatchRequest(this, msg->getMethod(), t, self, msg);
          break;
        }
        if (err != Error::INHIBIT) {
          msg->replyResponse(err);
          monitor.requestDone();
        }
      } );
  }

  Error ExampleObj::getDebugInfo(Cap self, IInvocation* msg)
  {
    mlogex.info("invoke getDebugInfo", DVAR(this), DVAR(self), DVAR(msg));
    return writeDebugInfo("ExampleObj", self, msg);
  }

  Error ExampleObj::printMessage(Tasklet*, Cap self, IInvocation* msg)
  {
    mlogex.info("invoke printMessage", DVAR(this), DVAR(self), DVAR(msg));
    auto data = msg->getMessage()->cast<protocol::Example::PrintMessage>();
    mlogex.error(mlog::DebugString(data->message, data->bytes));
    return Error::SUCCESS;
  }

  optional<ExampleObj*>
  ExampleFactory::factory(CapEntry* dstEntry, CapEntry* memEntry, Cap memCap,
                         IAllocator* mem)
  {
    auto obj = mem->create<ExampleObj>();
    if (!obj) return obj.state();
    Cap cap(*obj);
    auto res = cap::inherit(*memEntry, *dstEntry, memCap, cap);
    if (!res) {
      mem->free(*obj); // mem->release(obj) goes throug IKernelObject deletion mechanism
      return res.state();
    }
    return *obj;
  }

} // mythos
