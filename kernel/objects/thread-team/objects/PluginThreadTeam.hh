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
 * Copyright 2021 Philipp Gypser, BTU Cottbus-Senftenberg
 */
#pragma once

#include "objects/ThreadTeam.hh"
#include "util/events.hh"
#include "boot/load_init.hh"
#include "mythos/init.hh"
#include "boot/mlog.hh"
#include "util/assert.hh"

namespace mythos {

namespace factory {
  ThreadTeamFactory threadTeam;
}

  class PluginThreadTeamActivator
    : public EventHook<boot::InitLoader&>
  {
  public:
    PluginThreadTeamActivator() {
      event::initLoaderEarly.add(this);
    }
    virtual ~PluginThreadTeamActivator() {}

    void processEvent(boot::InitLoader& loader) override {
      MLOG_INFO(mlog::pm, "prevent mapping of all scheduling contexts into CSpace");
      loader.mapSchedulingContexts = false;
    }

  };

  class PluginThreadTeam
    : public EventHook<boot::InitLoader&>
    , public EventHook<ExecutionContext*>
  {
  public:
    PluginThreadTeam()
      : tt(nullptr)
    {
      event::initLoader.add(this);
      event::initEC.add(this);
    }
    virtual ~PluginThreadTeam() {}

    void processEvent(boot::InitLoader& loader) override {
      MLOG_DETAIL(mlog::pm, "init processor allocator");
      pa.init();
      MLOG_DETAIL(mlog::pm, "map processor allocator");
      OOPS(loader.csSet(init::PROCESSOR_ALLOCATOR, pa));
      MLOG_DETAIL(mlog::pm, "map thread team factory");
      OOPS(loader.csSet(init::THREADTEAM_FACTORY, factory::threadTeam));
      MLOG_DETAIL(mlog::pm, "get processor allocator");
      auto pae = loader.capAlloc.get(init::PROCESSOR_ALLOCATOR);
      ASSERT(pae);
      MLOG_DETAIL(mlog::pm, "create initial thread team");
      auto obj = loader.create<ThreadTeam, ThreadTeamFactory>(loader.capAlloc.get(init::THREAD_TEAM), *pae );
      if(obj){
        MLOG_DETAIL(mlog::pm, "thread team created successfully");
        tt = *obj;
      }else{
        MLOG_ERROR(mlog::pm, "ERROR: thread team creation failed!");
      }
    }

    void processEvent(ExecutionContext* ec) override {
        MLOG_DETAIL(mlog::pm, "runEc");
        ASSERT(tt != nullptr);
        tt->tryRun(ec);
    }

    ThreadTeam* tt;
    ProcessorAllocator pa;
  };

extern mythos::PluginThreadTeam pluginThreadTeam;
extern mythos::PluginThreadTeamActivator pluginThreadTeamActivator;
} // namespace mythos

