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

#include "objects/InterruptControl.hh"
#include "mythos/protocol/InterruptControl.hh"
#include "objects/mlog.hh"

namespace mythos {

static mlog::Logger<mlog::FilterAny> interruptLog("InterruptControl");

optional<void> InterruptControl::deleteCap(Cap self, IDeleter& del) {
    if (self.isOriginal()) {
        del.deleteObject(del_handle);
    }
    RETURN(Error::SUCCESS);
}

optional<void const*> InterruptControl::vcast(TypeId id) const {
    if (id == typeId<IKernelObject>()) return static_cast<IKernelObject const*>(this);
    THROW(Error::TYPE_MISMATCH);
}

void InterruptControl::invoke(Tasklet* t, Cap self, IInvocation* msg)
{
    monitor.request(t, [ = ](Tasklet * t) {
        Error err = Error::NOT_IMPLEMENTED;
        switch (msg->getProtocol()) {
            case protocol::KernelObject::proto:
                err = protocol::KernelObject::dispatchRequest(this, msg->getMethod(), self, msg);
                break;
            case protocol::InterruptControl::proto:
                err = protocol::InterruptControl::dispatchRequest(this, msg->getMethod(), t, self, msg);
                break;
        }
        if (err != Error::INHIBIT) {
            msg->replyResponse(err);
            monitor.requestDone();
        }
    } );
}

void InterruptControl::bind(optional<ISignalable*>) {
    interruptLog.error("bind", DVAR(this));
}

void InterruptControl::unbind(optional<ISignalable*>) {
    interruptLog.error("unbind", DVAR(this));
}

Error InterruptControl::getDebugInfo(Cap self, IInvocation* msg)
{
    return writeDebugInfo("InterruptControl", self, msg);
}

/**
 * TODO figure out:
 * What to do if there is already one registered?
 * Will only allow the first entity to register for the moment.
 */
Error InterruptControl::registerForInterrupt(Tasklet *t, Cap self, IInvocation *msg) {

    auto data = msg->getMessage()->read<protocol::InterruptControl::Register>();
    ASSERT(data.interrupt < 256);
    if (destinations[data.interrupt].isUsable()) {
        MLOG_INFO(mlog::boot, "Tried to register to an interrupt already taken:", data.interrupt);
        return Error::REQUEST_DENIED;
    }
    optional<CapEntry*> capEntry = msg->lookupEntry(data.ec());
    TypedCap<ISignalable> obj(capEntry);
    if (!obj) return Error::INVALID_CAPABILITY;
    destinations[data.interrupt].set(this, *capEntry, obj.cap());
    MLOG_ERROR(mlog::boot, "invoke registerForInterrupt", DVAR(t), DVAR(self), DVAR(data.ec()), DVAR(data.interrupt));
    return Error::SUCCESS;
}

Error InterruptControl::unregisterForInterrupt(Tasklet *t, Cap self, IInvocation *msg) {
    auto data = msg->getMessage()->read<protocol::InterruptControl::Register>();
    ASSERT(data.interrupt < 256);
    MLOG_ERROR(mlog::boot, "invoke unregisterForInterrupt", DVAR(self),DVAR(data.ec()), DVAR(data.interrupt));
    if (destinations[data.interrupt].isUsable()) {
        optional<CapEntry*> capEntry = msg->lookupEntry(data.ec());
        TypedCap<ISignalable> obj(capEntry);

        // TODO: this can probably be done better. Check if object to unregister is registered object in array
        if (obj && destinations[data.interrupt].cap().getPtr() != obj.cap().getPtr()) {
            MLOG_ERROR(mlog::boot, "Tried to unregister an ISignalable that wasnt registerd", DVAR(data.ec()));
            return Error::INVALID_ARGUMENT;
        }
    }
    destinations[data.interrupt].reset();
    ASSERT(!destinations[data.interrupt].isUsable());
    return Error::SUCCESS;
}

void InterruptControl::handleInterrupt(uint64_t interrupt) {
    ASSERT(interrupt < 256);
    if (!destinations[interrupt].isUsable()) {
        MLOG_ERROR(mlog::boot, "No one registered for", DVAR(interrupt));
        return;
    }

    TypedCap<ISignalable> ec(destinations[interrupt].cap());
    if (ec) {
        ec.obj()->signal((uint32_t)interrupt);
    }
    
}

} // namespace mythos