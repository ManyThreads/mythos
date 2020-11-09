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

#include "util/events.hh"
#include "boot/load_init.hh"
#include "mythos/init.hh"
#include "boot/mlog.hh"

namespace mythos {

  class PluginWithoutProcessorManagement
    : public EventHook<boot::InitLoader&>
  {
  public:
    PluginWithoutProcessorManagement() {
      event::initLoader.add(this);
    }
    virtual ~PluginWithoutProcessorManagement() {}

    void processEvent(boot::InitLoader& loader) override {
      ASSERT(cpu::getNumThreads() <= init::SCHEDULERS_START - init::APP_CAP_START);
      MLOG_INFO(mlog::boot, "... create scheduling context caps in caps",
            init::SCHEDULERS_START, "till", init::SCHEDULERS_START+cpu::getNumThreads()-1);
      for (cpu::ThreadID id = 0; id < cpu::getNumThreads(); ++id) {
        OOPS(loader.csSet(init::SCHEDULERS_START+id, boot::getScheduler(id)));
      }
    }
    
  };

PluginWithoutProcessorManagement pluginWithoutProcessorManagement;

} // namespace mythos
