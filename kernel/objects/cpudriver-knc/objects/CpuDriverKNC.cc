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
 * Copyright 2017 Randolf Rotta, Robert Kuban, and contributors, BTU Cottbus-Senftenberg
 */

#include "objects/CpuDriverKNC.hh"
#include "mythos/InvocationBuf.hh"
#include "util/assert.hh"
#include "boot/mlog.hh"
#include "mythos/protocol/CpuDriverKNC.hh"
#include "mythos/HostInfoTable.hh"
#include "objects/TypedCap.hh"
#include "objects/IFrame.hh"

namespace mythos {

  extern PhysPtr<HostInfoTable> hostInfoPtrPhys SYMBOL("_host_info_ptr");

  void CpuDriverKNC::invoke(Tasklet* t, Cap self, IInvocation* msg)
  {
    monitor.request(t, [=](Tasklet* t){
        Error err = Error::NOT_IMPLEMENTED;
        switch (msg->getProtocol()) {
        case protocol::CpuDriverKNC::proto:
          err = protocol::CpuDriverKNC::dispatchRequest(this, msg->getMethod(), t, self, msg);
          break;
        }
        if (err != Error::INHIBIT) {
          msg->replyResponse(err);
          monitor.requestDone();
        }
      } );
  }


  Error CpuDriverKNC::invoke_setInitMem(Tasklet*, Cap, IInvocation* msg)
  {
    auto data = msg->getMessage()->cast<protocol::CpuDriverKNC::SetInitMem>();

    /// @todo should be implemented similar to the CapRef to invocation buffer in Portals
    // however: revoking the capability does not prevent the host from continuing access

    TypedCap<IFrame> frame(msg->lookupEntry(data->capPtrs[0]));
    if (!frame) return frame;
    auto info = frame.getFrameInfo();
    if (info.device || !info.writable) return Error::INVALID_CAPABILITY;

    MLOG_INFO(mlog::boot, "invoke setInitMem", DVAR(data->capPtrs[0]),
              DVARhex(info.start.physint()), DVAR(info.size));

    auto hostInfoPtr = PhysPtr<PhysPtr<HostInfoTable>>::fromPhys(&hostInfoPtrPhys);
    auto hostInfo = *hostInfoPtr;
    if (!hostInfo) return Error::INSUFFICIENT_RESOURCES;
    hostInfo->initMem = info.start.physint();
    hostInfo->initMemSize = info.size;

    return Error::SUCCESS;
  }

} // namespace mythos
