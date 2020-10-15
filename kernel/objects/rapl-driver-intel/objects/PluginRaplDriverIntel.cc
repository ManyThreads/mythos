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

#include "objects/RaplDriverIntel.hh"
#include "util/events.hh"
#include "boot/load_init.hh"
#include "mythos/init.hh"
#include "boot/mlog.hh"

namespace mythos {

  class PluginRaplDriverIntel
    : public EventHook<boot::InitLoader&>
  {
  public:
    PluginRaplDriverIntel() {
      event::initLoader.add(this);
    }
    virtual ~PluginRaplDriverIntel() {}

    void processEvent(boot::InitLoader& loader) override {
      OOPS(loader.csSet(init::RAPL_DRIVER_INTEL, rapldrv));
    }
    
    RaplDriverIntel rapldrv;
  };

  PluginRaplDriverIntel pluginRaplDriverIntel;

} // namespace mythos
