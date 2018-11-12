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

#include <cstdint>
#include <cstddef>
#include "util/PhysPtr.hh"
#include "util/optional.hh"
#include "objects/Cap.hh"
#include "mythos/protocol/PageMap.hh"
#include "objects/TypedCap.hh"

namespace mythos {

  class CapEntry;

  class IPageMap
  {
  public:
    typedef protocol::PageMap::MapFlags MapFlags;
    virtual ~IPageMap() {}

    struct Info {
      bool isRootMap() const { return level==4; }
      size_t level;
      PhysPtr<void> table;
      bool configurable;
    };

    virtual Info getPageMapInfo(Cap self) const = 0;

    // for testing
    virtual void print() const = 0;

    virtual optional<void> mapFrame(uintptr_t vaddr, size_t size, CapEntry* frameEntry, MapFlags flags, uintptr_t offset, 
                                   uintptr_t* failaddr, size_t* faillevel) = 0; 
    virtual optional<void> mapTable(uintptr_t vaddr, size_t level, CapEntry* tableEntry, MapFlags flags,
                                    uintptr_t* failaddr, size_t* faillevel) = 0;
    
    virtual optional<void> mapFrame(size_t index, CapEntry* frameEntry, MapFlags flags, size_t offset) = 0;
    virtual optional<void> unmapEntry(size_t index) = 0;

    virtual optional<void> mapTable(CapEntry* table, MapFlags req, size_t index) = 0;
  };

} // namespace mythos
