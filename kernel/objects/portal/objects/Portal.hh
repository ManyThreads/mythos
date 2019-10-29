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

#include "mythos/InvocationBuf.hh"
#include "util/assert.hh"
#include "async/IResult.hh"
#include "objects/IPortal.hh"
#include "objects/IFrame.hh"
#include "objects/IFactory.hh"
#include "async/NestedMonitorDelegating.hh"
#include "objects/CapRef.hh"
#include "objects/mlog.hh"
#include "objects/RevokeOperation.hh"
#include "mythos/protocol/Portal.hh"
#include "util/Logger.hh"

namespace mlog {
#ifndef MLOG_PORTAL
#define MLOG_PORTAL FilterError
#endif
  extern mlog::Logger<MLOG_PORTAL> portal;
}


namespace mythos {

  /** Portal for sending and receiving invokation messages.  Portals
   * own an InvocationBuf as storage space for incoming and outgoing
   * message data. Access to this buffer is shared with the user space
   * via a mapped frame capability.
   *
   * @todo split into Portal for sending-only and EndPoint for
   * receiving-only. They have quite much in common but end points can
   * be much simpler (no IInvocation, no deletion service, but
   * badges). Mixed use of portals for sending and receiving is not
   * useful anyway! Check the design: End points may use the
   * ressources of an associated Portal instead of being associated
   * with an ExecutionContext.
   *
   * @todo Enqueing to an endpoint or receiver portal has the
   * advantage, that we know the receiver has an own invocation
   * buffer. Instead of pulling data through the enqueued IInvocation
   * from the possibly non-existent sender's buffer, we should tell
   * the IInvocation where to put its data. This simplifies the
   * exception forwarding stuff.
   */
  class Portal final
    : public IKernelObject
    , public IPortal
    , public IInvocation
    , public INotification
    , public IResult<void>
  {
  public:
    enum State : uint8_t { OPEN, INVOKING, REPLYING };

    Portal(IAsyncFree* memory) : memory(memory) {}
    virtual ~Portal() {}

    optional<void> setInvocationBuf(optional<CapEntry*> frame, uint32_t offset);
    void unsetInvocationBuf() { _ib.reset(); }
    optional<void> setOwner(optional<CapEntry*> ec);
    void unsetOwner() { _owner.reset(); }

  public: // INotification interface
    KEvent deliver() override {
      ASSERT(portalState == REPLYING);
      KEvent e(uctx, uint64_t(replyError));
      portalState = OPEN;
      return e;
    }

  public: // IPortal interface
    optional<void> sendInvocation(Cap self, CapPtr dest, uint64_t uctx) override;

  public: // IInvocation interface
    void replyResponse(optional<void> error) override;

    void deletionResponse(CapEntry*, bool) override;

    /** IResponse<void> interface, currently only used for deletionResponse */
    virtual void response(Tasklet*, optional<void>) override;

    CapEntry* getCapEntry() const override { return currentDest.entry; }
    Cap getCap() const override { return destCap; }
    uint16_t getLabel() const override { return ib? uint16_t(ib->tag.label) : uint16_t(-1); }
    InvocationBuf* getMessage() const override { return ib; }
    size_t getMaxSize() const override { return 480; }
    optional<CapEntryRef> lookupRef(CapPtr ptr, CapPtrDepth ptrDepth, bool writable) override;
    optional<CapEntry*> lookupEntry(CapPtr ptr, CapPtrDepth ptrDepth, bool writable) override;

  public: // IKernelObject interface

    optional<void const*> vcast(TypeId id) const override {
      if (id == typeId<IPortal>()) return static_cast<IPortal const*>(this);
      THROW(Error::TYPE_MISMATCH);
    }
    optional<void> deleteCap(CapEntry&, Cap self, IDeleter& del) override;

    void deleteObject(Tasklet* t, IResult<void>* r) override {
      monitor.doDelete(t, [=](Tasklet* t) { memory->free(t, r, this, sizeof(Portal)); });
    }

    void invoke(Tasklet* t, Cap, IInvocation* msg) override;
    Error invokeBind(Tasklet* t, Cap, IInvocation* msg);

  protected:
    friend class CapRefBind;
    void bind(optional<IFrame*>);
    void bind(optional<IPortalUser*>) {}
    void unbind(optional<IFrame*>);
    void unbind(optional<IPortalUser*>);

  private:
    CapRef<Portal, IFrame> _ib;
    InvocationBuf* ib = nullptr;
    InvocationBuf* ibNew = nullptr;
    CapRef<Portal, IPortalUser> _owner;
    uintptr_t uctx = 0;
    INotifiable::handle_t notificationHandle = {this};

    Tasklet mytask;

    std::atomic<uint8_t> portalState = {OPEN};
    Error replyError;
    CapEntryRef currentDest;
    Cap destCap;

    /** list handle for the deletion procedure */
    LinkedList<IKernelObject*>::Queueable del_handle = {this};
    IAsyncFree* memory;
    RevokeOperation revokeOp = {monitor};
    Tasklet rootTask; //< start of one logical control flow per portal
    async::NestedMonitorDelegating monitor;
  };

  class PortalFactory : public FactoryBase
  {
  public:
    static optional<Portal*>
    factory(CapEntry* dstEntry, CapEntry* memEntry, Cap memCap, IAllocator* mem);

    Error factory(CapEntry* dstEntry, CapEntry* memEntry, Cap memCap,
                  IAllocator* mem, IInvocation*) const override {
      return factory(dstEntry, memEntry, memCap, mem).state();
    }
  };

} // namespace mythos
