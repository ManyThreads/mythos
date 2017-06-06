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
 * Copyright 2016 Randolf Rotta, Robert Kuban, and contributors, BTU Cottbus-Senftenberg
 */
#pragma once

#include <cstdint>
#include "util/optional.hh"
#include "objects/Cap.hh"
#include "mythos/protocol/common.hh"
#include "objects/ICapMap.hh"

namespace mythos {

  class Tasklet;
  class IPortalUser;
  class InvocationBuf;
  class IPortal;
  class CapEntry;

  class IInvocation
  {
  public:
    /** tells the invoker synchronously, that the processing has
     * finished and the response is written into the message. The
     * tasklet that was passed for the invocation is implicitly
     * released.
     */
    virtual void replyResponse(optional<void> error) = 0;
    inline void replyResponse(Error err) { replyResponse(optional<void>(err)); };

    /** called by the target kernel object in response to having
     * completed the message processing. Tells the portal to initiate
     * a capability revokation and object deletion. */
    virtual void deletionResponse(CapEntry* c, bool delRoot) = 0;

    /** returns the capability entry that was used to address the
     * receiving kernel object. */
    virtual CapEntry* getCapEntry() const = 0;

    /** returns the capability value that was used to address the
     * receiving kernel object. */
    virtual Cap getCap() const = 0;

    /** returns the invocation message label or 0xFFFF if no buffer is present. */
    virtual uint16_t getLabel() const = 0;

    uint8_t getMethod() const { return uint8_t(getLabel()); }
    uint8_t getProtocol() const { return uint8_t(getLabel() >> 8); }

    /** access the invocation buffer */
    virtual InvocationBuf* getMessage() const = 0;

    /** maximum number of bytes in the raw message data. used when
     * writing responses into the message. usually it is 480 bytes. */
    virtual size_t getMaxSize() const = 0;

    virtual optional<CapEntryRef> lookupRef(CapPtr ptr, CapPtrDepth ptrDepth=32, bool writable=false) = 0;
    virtual optional<CapEntry*> lookupEntry(CapPtr ptr, CapPtrDepth ptrDepth=32, bool writable=false) = 0;
  };

} // namespace mythos
