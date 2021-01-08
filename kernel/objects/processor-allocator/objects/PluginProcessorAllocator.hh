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
 * Copyright 2020 Philipp Gypser and contributors, BTU Cottbus-Senftenberg
 */
#pragma once

#include "objects/ProcessorAllocator.hh"
#include "util/events.hh"
#include "boot/load_init.hh"
#include "mythos/init.hh"
#include "boot/mlog.hh"
#include "util/assert.hh"

namespace mythos {
  class PluginProcessorAllocatorActivator
    : public EventHook<boot::InitLoader&>
  {
  public:
    PluginProcessorAllocatorActivator() {
      event::initLoaderEarly.add(this);
    }
    virtual ~PluginProcessorAllocatorActivator() {}

    void processEvent(boot::InitLoader& loader) override {
      MLOG_DETAIL(mlog::pm, "prevent mapping of all scheduling contexts into CSpace");
      loader.processorAllocatorPresent = true;
    }
  };

  class PluginProcessorAllocator
    : public EventHook<boot::InitLoader&>
  {
  public:
    PluginProcessorAllocator() {
      event::initLoader.add(this);
    }
    virtual ~PluginProcessorAllocator() {}

    void processEvent(boot::InitLoader& loader) override {
      MLOG_DETAIL(mlog::pm, "Init processor allocator");
      pa.init();
      MLOG_DETAIL(mlog::pm, "CSset processor allocator");
      OOPS(loader.csSet(init::PROCESSOR_ALLOCATOR, pa));
      auto sc = pa.alloc();
      ASSERT(sc); 
      MLOG_DETAIL(mlog::pm, "allocated SC for init app", DVAR(init::SCHEDULERS_START+*sc));
      loader.initSC = init::SCHEDULERS_START+*sc;
      MLOG_DETAIL(mlog::pm, "map SC for init app");
      loader.csSet(init::SCHEDULERS_START+*sc, boot::getScheduler(*sc));

    }

    LiFoProcessorAllocator pa;
  };

} // namespace mythos

extern mythos::PluginProcessorAllocator pluginProcessorAllocator;
extern mythos::PluginProcessorAllocatorActivator pluginProcessorAllocatorActivator;
