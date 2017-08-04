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
 * Copyright 2014 Randolf Rotta, Maik Krüger, and contributors, BTU Cottbus-Senftenberg
 */

#include "boot/mlog.hh"
#include "cpu/hwthreadid.hh"
#include "boot/memory-layout.h"
#include "util/mstring.hh"
#include "mythos/HostInfoTable.hh"
#include "mythos/PciMsgQueueMPSC.hh"
#include <new>

namespace mythos {

  class MLogSinkPci
    : public mlog::ISink
  {
  public:
    typedef HostInfoTable::DebugChannel handle_t;
    MLogSinkPci(HostInfoTable::DebugChannel* channel) : channel(channel) {}
    virtual ~MLogSinkPci() {}
    void write(char const* msg, size_t length) override;
    void flush() override {}
  protected:
    void sendPkg(char const* msg, uint16_t vchannel, size_t msgbytes);
    PCIeRingProducer<HostInfoTable::DebugChannel> channel;
  };

  void MLogSinkPci::sendPkg(char const* msg, uint16_t vchannel, size_t msgbytes)
  {
    auto handle = channel.acquireSend();
    auto& pkg = channel.get<DebugMsg>(handle);
    pkg.vchannel = vchannel;
    pkg.msgbytes = uint16_t(msgbytes);
    if (msgbytes > DebugMsg::PAYLOAD) {
      memcpy(pkg.data, msg, DebugMsg::PAYLOAD);
      channel.finishSend(handle, DebugMsg::PAYLOAD);
    } else {
      memcpy(pkg.data, msg, msgbytes);
      channel.finishSend(handle, msgbytes);
    }
  }

  void MLogSinkPci::write(char const* msg, size_t length)
  {
    uint16_t vchannel = cpu::getThreadID();
    while (length>DebugMsg::PAYLOAD) {
      sendPkg(msg, vchannel, length);
      msg += DebugMsg::PAYLOAD;
      length -= DebugMsg::PAYLOAD;
    }
    sendPkg(msg, vchannel, length);
  }

  namespace boot {
    extern HostInfoTable::DebugChannel debugOut;
    alignas(MLogSinkPci) char mlogsink[sizeof(MLogSinkPci)];

    void initMLog()
    {
      // need constructor for vtable before global constructors were called
      auto s = new(&mlogsink) MLogSinkPci(&debugOut);
      mlog::sink = s; // now logging is working
      mlog::boot.setName("boot"); // because constructor is still not called
    }
  } // namespace boot
} // namespace mythos


namespace mlog {
  Logger<MLOG_BOOT> boot("boot");
} // namespace mlog
