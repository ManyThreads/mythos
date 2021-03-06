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
 * Copyright 2019 Randolf Rotta and contributors, BTU Cottbus-Senftenberg
 */
#include "boot/load_init.hh"
#include "boot/kernel.hh"
#include "util/events.hh"

namespace mythos {

extern char app_image_start SYMBOL("app_image_start");

/** create the root task EC on the first hardware thread */
class InitLoaderPlugin
   : public EventHook<cpu::ThreadID, bool, size_t>
{
public:
    InitLoaderPlugin() { event::bootAP.add(this); }

    void processEvent(cpu::ThreadID threadID, bool firstBoot, size_t) override {
        if (threadID == 0 && firstBoot) {
            OOPS(boot::InitLoader(&app_image_start).load());
        }
    }
};

InitLoaderPlugin initloaderplugin;

} // namespace mythos
