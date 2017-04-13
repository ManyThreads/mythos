/* -*- mode:C++; indent-tabs-mode:nil; -*- */
/* MyThOS: The Many-Threads Operating System
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

namespace mythos {

  class CapEntry;
  struct CapEntryRef;

  class ICapMap
  {
  public:
    virtual ~ICapMap() {}

    /** queries for a member of capability containers and return a
     * pointer to the requested capability entry.  This operation is
     * supported by containers like the CNode if accessed through a
     * capability with lookup rights.
     * Default implementation returns Error::NO_LOOKUP.
     *
     * @param self      the capability used to access this map.
     * @param ptr       the capability pointer that should be looked up.
     * @param ptrDepth  the number of remaining valid bits in the capability
     *                  pointer. This is needed for recursive lookups.
     * @param writable  set to true if the found entry shall be modified.
     * @return is empty if the object is no container, the access right
     *         were insufficient, or the pointer was outside the container.
     */
    virtual optional<CapEntryRef> lookup(Cap self, CapPtr ptr, CapPtrDepth ptrDepth=32, bool writable=false) = 0;

    virtual void acquireEntryRef() = 0;
    virtual void releaseEntryRef() = 0;
  };

  struct CapEntryRef
  {
    CapEntryRef() : entry(nullptr), container(nullptr) {}
    CapEntryRef(CapEntry* entry, ICapMap* container) : entry(entry), container(container) {}
    bool operator! () const { return entry == nullptr; }
    explicit operator bool() const { return entry != nullptr; }
    void acquire() { container->acquireEntryRef(); }
    void release() {
      if (container) container->releaseEntryRef();
      entry = nullptr;
      container = nullptr;
    }
    CapEntry* entry;
    ICapMap* container;
  };

} // namespace mythos
