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

#include <new>
#include "mythos/InvocationBuf.hh"
#include "util/assert.hh"
#include "util/PhysPtr.hh"
#include "util/alignments.hh"
#include "objects/ExampleHome.hh"
#include "objects/ops.hh"
#include "objects/KernelMemory.hh"
#include "boot/memory-root.hh"
#include "objects/DebugMessage.hh"
#include "util/error-trace.hh"

namespace mythos {

  static mlog::Logger<mlog::FilterAny> mlogexhome("ExampleHomeObj");

  optional<void const*> ExampleHomeObj::vcast(TypeId id) const
  {
	mlogexhome.info("vcast", DVAR(this), DVAR(id.debug()));
    if (id == typeId<ExampleHomeObj>()) return /*static_cast<ExampleObj const*>*/(this);
    THROW(Error::TYPE_MISMATCH);
  }

  optional<void> ExampleHomeObj::deleteCap(Cap self, IDeleter& del)
  {
	mlogexhome.info("deleteCap", DVAR(this), DVAR(self), DVAR(self.isOriginal()));
    if (self.isOriginal()) {
      del.deleteObject(del_handle);
    }
    RETURN(Error::SUCCESS);
  }

  void ExampleHomeObj::deleteObject(Tasklet* t, IResult<void>* r)
  {
	mlogexhome.info("deleteObject", DVAR(this), DVAR(t), DVAR(r));
    monitor.doDelete(t, [=](Tasklet* t){
      _mem->free(t, r, this, sizeof(ExampleObj));
    });
  }

  void ExampleHomeObj::invoke(Tasklet* t, Cap self, IInvocation* msg)
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

  Error ExampleHomeObj::getDebugInfo(Cap self, IInvocation* msg)
  {
	mlogexhome.info("invoke getDebugInfo", DVAR(this), DVAR(self), DVAR(msg));
    return writeDebugInfo("ExampleHomeObj", self, msg);
  }

  Error ExampleHomeObj::printMessage(Tasklet*, Cap self, IInvocation* msg)
  {
	mlogexhome.info("invoke printMessage", DVAR(this), DVAR(self), DVAR(msg));
    auto data = msg->getMessage()->cast<protocol::Example::PrintMessage>();
    mlogexhome.error(mlog::DebugString(data->message, data->bytes));
    return Error::SUCCESS;
  }

  Error ExampleHomeObj::ping(Tasklet*, Cap self, IInvocation* msg)
  {
	mlogexhome.info("invoke ping", DVAR(this), DVAR(self), DVAR(msg));
	auto data = msg->getMessage()->cast<protocol::Example::Ping>();
	uint64_t wait_cycles = data->wait_cycles;

	for(uint64_t count = 0; count < wait_cycles; count++);

	data->place = cpu::hwThreadID_;

	return Error::SUCCESS;
  }

  Error ExampleHomeObj::moveHome(Tasklet*, Cap self, IInvocation* msg)
  {
	mlogexhome.info("invoke moveHome", DVAR(this), DVAR(self), DVAR(msg));
	auto data = msg->getMessage()->cast<protocol::Example::MoveHome>();
	mlogexhome.error(DVAR(data->location));
	monitor.setHome(&async::places[data->location]);
	return Error::SUCCESS;
  }

  optional<ExampleHomeObj*>
  ExampleHomeFactory::factory(CapEntry* dstEntry, CapEntry* memEntry, Cap memCap,
                         IAllocator* mem)
  {
    auto obj = mem->create<ExampleHomeObj>();
    if (!obj) RETHROW(obj);
    Cap cap(*obj);
    auto res = cap::inherit(*memEntry, *dstEntry, memCap, cap);
    if (!res) {
      mem->free(*obj); // mem->release(obj) goes throug IKernelObject deletion mechanism
      RETHROW(res);
    }
    return *obj;
  }

} // mythos
