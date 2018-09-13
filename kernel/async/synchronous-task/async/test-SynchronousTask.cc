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
 * Copyright 2018 Randolf Rotta, and contributors, BTU Cottbus-Senftenberg
 */

#include "plugins/Plugin.hh"
#include "async/Place.hh"
#include "async/SynchronousTask.hh"
#include "boot/mlog.hh"
#include "cpu/hwthreadid.hh"

namespace mythos {

class TestSynchronousTask
    : public Plugin
{
public:
    void initGlobal() { }

    void initThread(cpu::ThreadID threadID)
    {
        MLOG_INFO(this->log, "TestSynchronousTask: my place is", 
                    &mythos::async::getLocalPlace(),
                    DVAR(threadID), cpu::getNumThreads());

        for (int i=0; i<1000; i++) {
            for (mythos::cpu::ThreadID dst=0; dst<cpu::getNumThreads(); dst++) {
                synchronousAt(async::getPlace(dst)) << [threadID,dst,this]() {
                    //MLOG_DETAIL(this->log, "TestSynchronousTask: msg",
                    //            DVAR(threadID), DVAR(dst), DVAR(cpu::getThreadID()));
                };
            }
        }
        MLOG_INFO(this->log, "TestSynchronousTask: done");
    }

};

TestSynchronousTask plugin_TestSynchronousTask;

} // namespace mythos
