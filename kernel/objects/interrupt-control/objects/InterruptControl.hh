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

#include "objects/IFactory.hh"
#include "objects/CapRef.hh"
#include "objects/ISignalable.hh"
#include "mythos/protocol/KernelObject.hh"

namespace mythos {


class InterruptControl
    : public IKernelObject
{
public:
    InterruptControl()
        {}
public: // IKernelObject interface
    optional<void const*> vcast(TypeId id) const override;
    optional<void> deleteCap(Cap self, IDeleter& del) override;
    void invoke(Tasklet* t, Cap, IInvocation* msg) override;
    Error invokeBind(Tasklet* t, Cap, IInvocation* msg);
public: // protocol 
    friend struct protocol::KernelObject;
    Error getDebugInfo(Cap self, IInvocation* msg);
    Error registerForInterrupt(Tasklet *t, Cap self, IInvocation *msg);
    Error unregisterForInterrupt(Tasklet *t, Cap self, IInvocation *msg);
public:
    void bind(optional<ISignalable*>);
    void unbind(optional<ISignalable*>);
public:
    void handleInterrupt(uint64_t interrupt);
    void maskIRQ(uint64_t interrupt);
    void ackIRQ(uint64_t interrupt);
private:
    bool isValid(uint64_t interrupt) { return interrupt < 256 && interrupt > 31; }
private:
    /** list handle for the deletion procedure */
    LinkedList<IKernelObject*>::Queueable del_handle = {this};
    async::NestedMonitorDelegating monitor;

    // actual interrupt handling members
    CapRef<InterruptControl, ISignalable> destinations[256];
};

} // namespace mythos
