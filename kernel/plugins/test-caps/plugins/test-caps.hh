#pragma once

#include "async/IResult.hh"
#include "async/NestedMonitorDelegating.hh"
#include "objects/CapEntry.hh"
#include "objects/RevokeOperation.hh"
#include "plugins/TestPlugin.hh"

namespace mythos {
class UntypedMemory;
namespace test_caps {

class TestCaps : public TestPlugin, public IResult<void>
  {
  public:
    TestCaps();
    void printCaps();
    virtual void initThread(size_t /*threadid*/) override;
    virtual void initGlobal() override;

    virtual void response(Tasklet* t, optional<void> res) override;

  private:
    CapEntry& root_cap;
    CapEntry* test_caps;
    CapEntry _caps[10];

    size_t state;
    void proto();

    Tasklet tasklet;
    async::NestedMonitorDelegating monitor;
    RevokeOperation op;
  };

} // test_caps
} // mythos
