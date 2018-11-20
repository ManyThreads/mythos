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
#include "cpu/IOApic.hh"

namespace mythos {

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
}

void InterruptControl::unbind(optional<ISignalable*>) {
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
    if (!isValid(data.interrupt)) {
        return Error::INVALID_ARGUMENT;
    }
    if (destinations[data.interrupt].isUsable()) {
        MLOG_INFO(mlog::irq, "Tried to register to an interrupt already taken:", data.interrupt);
        return Error::REQUEST_DENIED;
    }
    optional<CapEntry*> capEntry = msg->lookupEntry(data.ec());
    TypedCap<ISignalable> obj(capEntry);
    if (!obj) return Error::INVALID_CAPABILITY;
    destinations[data.interrupt].set(this, *capEntry, obj.cap());
    MLOG_DETAIL(mlog::irq, "registerForInterrupt", DVAR(t), DVAR(self), DVAR(data.ec()), DVAR(data.interrupt));
    return Error::SUCCESS;
}

Error InterruptControl::unregisterForInterrupt(Tasklet*/* t*/, Cap self, IInvocation *msg) {
    auto data = msg->getMessage()->read<protocol::InterruptControl::Unregister>();
    if (!isValid(data.interrupt)) {
        return Error::INVALID_ARGUMENT;
    }
    MLOG_DETAIL(mlog::irq, "unregisterForInterrupt", DVAR(self), DVAR(data.ec()), DVAR(data.interrupt));
    if (destinations[data.interrupt].isUsable()) {
        optional<CapEntry*> capEntry = msg->lookupEntry(data.ec());
        TypedCap<ISignalable> obj(capEntry);

        // TODO: this can probably be done better. Check if object to unregister is registered object in array
        if (obj && destinations[data.interrupt].cap().getPtr() != obj.cap().getPtr()) {
            MLOG_DETAIL(mlog::irq, "Tried to unregister an ISignalable that wasnt registerd", DVAR(data.interrupt), DVAR(data.ec()));
            return Error::INVALID_ARGUMENT;
        }
    }
    destinations[data.interrupt].reset();
    ASSERT(!destinations[data.interrupt].isUsable());
    return Error::SUCCESS;
}

Error InterruptControl::maskIRQ(Tasklet* /*t*/, Cap /*self*/, IInvocation *msg) {
    auto data = msg->getMessage()->read<protocol::InterruptControl::MaskIRQ>();
    if (!isValid(data.interrupt)) {
        return Error::INVALID_ARGUMENT;
    }
    maskIRQ(data.interrupt);
    return Error::SUCCESS;
}

Error InterruptControl::unmaskIRQ(Tasklet* /*t*/, Cap /*self*/, IInvocation *msg) {
    auto data = msg->getMessage()->read<protocol::InterruptControl::MaskIRQ>();
    if (!isValid(data.interrupt)) {
        return Error::INVALID_ARGUMENT;
    }
    unmaskIRQ(data.interrupt);
    return Error::SUCCESS;
}

void InterruptControl::handleInterrupt(uint64_t interrupt) {
    ASSERT(isValid(interrupt));
    if (interrupt == 32) { // just acknowledge IPI
        MLOG_DETAIL(mlog::irq, "Acknowledge received IPI");
        mythos::lapic.endOfInterrupt();
        return;
    }

    if (!destinations[interrupt].isUsable()) {
        MLOG_WARN(mlog::irq, "No one registered for", DVAR(interrupt));
        mythos::lapic.endOfInterrupt();
        return;
    }
    maskIRQ(interrupt);
    mythos::lapic.endOfInterrupt();
    TypedCap<ISignalable> signalable(destinations[interrupt].cap());
    if (signalable) {
        signalable.obj()->signal((uint32_t) interrupt);
    }
}

void InterruptControl::maskIRQ(uint64_t interrupt) {
    ASSERT(isValid(interrupt));
    ioapic.maskIRQ(interrupt);
}

void InterruptControl::unmaskIRQ(uint64_t interrupt) {
    ASSERT(isValid(interrupt));
    ioapic.unmaskIRQ(interrupt);
}

} // namespace mythos