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
    virtual void initThread(cpu::ThreadID threadID) override;
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
