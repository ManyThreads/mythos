/* -*- mode:C++; -*- */
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
 * Copyright 2016 Randolf Rotta, Robert Kuban, Maik Krüger, and contributors, BTU Cottbus-Senftenberg
 */

#include "util/events.hh"
#include "boot/kernel.hh"
#include "plugins/Plugin.hh"
#include "boot/mlog.hh"

namespace mythos {

  Plugin* Plugin::first = nullptr;

  Plugin::Plugin(const char* name) : log(name)
  {
    this->next = first;
    first = this;
  }

  class InitPlugins : public EventHook<cpu::ThreadID, bool, size_t>
  {
  public:
    InitPlugins() { bootAPEvent.add(this); }

    EventCtrl before(cpu::ThreadID, bool, size_t) override {
      for (Plugin* c = Plugin::first; c != nullptr; c = c->next) {
        MLOG_DETAIL(mlog::boot, "initPluginsGlobal", c);
        c->initGlobal();
      }
      return EventCtrl::OK;
    }

    EventCtrl after(cpu::ThreadID threadID, bool firstBoot, size_t) override {
      if (firstBoot) {
        for (Plugin* c = Plugin::first; c != nullptr; c = c->next) {
          MLOG_DETAIL(mlog::boot, "initPluginsOnThread", c, threadID);
          c->initThread(threadID);
        }
      }
      return EventCtrl::OK;
    }
  };

  InitPlugins initPluginsPlugin;

} // namespace mythos
