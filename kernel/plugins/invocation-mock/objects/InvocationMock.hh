#pragma once

#include "objects/IExecutionContext.hh"
#include "objects/IInvocation.hh"

namespace mythos {

  class InvocationMock : public IInvocation
  {
  public:

    InvocationMock(CapEntry& entry) : _entry(&entry), _cap(entry.cap()) {}
    InvocationMock(CapEntry& entry, Cap cap) : _entry(&entry), _cap(cap) {}

    virtual void replyResponse(optional<void>) override { ASSERT(!"NOT IMPLEMENTED"); }
    virtual void enqueueResponse(LinkedList<IInvocation*>&, IPortal*) override { ASSERT(!"NOT IMPLEMENTED"); }
    virtual void deletionResponse(CapEntry*, bool) override { ASSERT(!"NOT IMPLEMENTED"); }

    virtual Cap getCap() const override { return _cap; }
    virtual CapEntry* getCapEntry() const override { return _entry; }

    virtual IExecutionContext* getEC() override { ASSERT(!"NOT IMPLEMENTED"); }
    virtual InvocationBuf* getMessage()  override { return nullptr; }

    virtual size_t getMaxSize() override { return 0; }

    IInvocation* addr() { return this; };

  protected:
    CapEntry* _entry;
    Cap _cap;
  };

}
