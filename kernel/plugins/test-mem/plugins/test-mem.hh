#pragma once

#include "async/IResult.hh"
#include "async/NestedMonitorDelegating.hh"
#include "objects/CapEntry.hh"
#include "objects/RevokeOperation.hh"
#include "plugins/TestPlugin.hh"
#include "objects/IPageMap.hh"

namespace mythos {
namespace test_mem {

class TestMem : public TestPlugin, public IResult<void>
  {
  public:
    TestMem();
    void printCaps();
    virtual void initThread(size_t /*threadid*/) override;
    virtual void initGlobal() override;

    virtual void response(Tasklet* t, optional<void> res) override;

  private:
    CapEntry* caps;
    CapEntry _caps[20];

    size_t state;
    void proto();

    Tasklet tasklet;
    async::NestedMonitorDelegating monitor;
    RevokeOperation op;

    IPageMap* pml4map;
    IPageMap* pml3map;
    IPageMap* pml2map;
    IPageMap* pml1map;

    void createRegions();
    void createFrames();
    void createMaps();
    void createMappings();
  };

} // test_mem
} // mythos
