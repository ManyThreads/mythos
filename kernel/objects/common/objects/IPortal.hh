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
 * Copyright 2016 Randolf Rotta, Robert Kuban, and contributors, BTU Cottbus-Senftenberg
 */
#pragma once

#include "mythos/caps.hh"
#include "util/optional.hh"
#include "objects/Cap.hh"
#include "objects/ICapMap.hh"
#include "objects/INotifiable.hh"
#include "objects/TypedCap.hh"

namespace mythos {

  class IPortalUser;
  class ICapMap;
  class CapEntry;
  
  class IPortal
  {
  public:
    virtual ~IPortal() {}

    /** the content of the invocation buffer is sent asynchronously as
     * request to the specific destination. In
     * general, any kernel object that implements the invoke operation
     * is a valid destination. When used for communication between
     * applications, the destination will be a Portal. 
     *
     * @param dest The destination kernel object that handles the request.
     */
    virtual optional<void> sendInvocation(Cap self, CapPtr dest, uint64_t user) = 0;
  };

  class IPortalUser
    : public INotifiable
  {
  public:
    virtual ~IPortalUser() {}

    virtual optional<CapEntryRef> lookupRef(CapPtr ptr, CapPtrDepth ptrDepth=32, bool writable=false) = 0;
  };
  
} // namespace mythos
