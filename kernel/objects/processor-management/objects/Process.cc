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
 * Copyright 2020 Philipp Gypser and contributors, BTU Cottbus-Senftenberg
 */
#include <new>
#include "mythos/InvocationBuf.hh"
#include "util/assert.hh"
#include "util/PhysPtr.hh"
#include "objects/Process.hh"
#include "objects/ops.hh"
#include "objects/KernelMemory.hh"
#include "boot/memory-root.hh"
#include "objects/DebugMessage.hh"
#include "util/error-trace.hh"

namespace mythos {

  static mlog::Logger<mlog::FilterAny> mlogp("Process");

  optional<void const*> Process::vcast(TypeId id) const
  {
    mlogp.info("vcast", DVAR(this), DVAR(id.debug()));
    if (id == typeId<Process>()) return /*static_cast<Process const*>*/(this);
    THROW(Error::TYPE_MISMATCH);
  }

  optional<void> Process::deleteCap(CapEntry&, Cap self, IDeleter& del)
  {
    mlogp.info("deleteCap", DVAR(this), DVAR(self), DVAR(self.isOriginal()));
    if (self.isOriginal()) {
      del.deleteObject(del_handle);
    }
    RETURN(Error::SUCCESS);
  }

  void Process::deleteObject(Tasklet* t, IResult<void>* r)
  {
    mlogp.info("deleteObject", DVAR(this), DVAR(t), DVAR(r));
    monitor.doDelete(t, [=](Tasklet* t){
      _mem->free(t, r, this, sizeof(Process));
    });
  }

  void Process::invoke(Tasklet* t, Cap self, IInvocation* msg)
  {
    monitor.request(t, [=](Tasklet* t){
        Error err = Error::NOT_IMPLEMENTED;
        switch (msg->getProtocol()) {
        case protocol::KernelObject::proto:
          err = protocol::KernelObject::dispatchRequest(this, msg->getMethod(), self, msg);
          break;
        case protocol::Process::proto:
          err = protocol::Process::dispatchRequest(this, msg->getMethod(), t, self, msg);
          break;
        }
        if (err != Error::INHIBIT) {
          msg->replyResponse(err);
          monitor.requestDone();
        }
      } );
  }

  Error Process::getDebugInfo(Cap self, IInvocation* msg)
  {
    mlogp.info("invoke getDebugInfo", DVAR(this), DVAR(self), DVAR(msg));
    return writeDebugInfo("Process", self, msg);
  }

  optional<Process*>
  ProcessFactory::factory(CapEntry* dstEntry, CapEntry* memEntry, Cap memCap,
                         IAllocator* mem)
  {
    auto obj = mem->create<Process>();
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
