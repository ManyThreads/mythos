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
#pragma once

#include "boot/CapAlloc.hh"
#include "objects/CapEntry.hh"
#include "objects/IPageMap.hh"

namespace mythos {

class KernelMemory;

class MemMapper
{
public:
    enum Config {
        MAX_PAGE_MAPS = 20,
    };

    struct MapInfo
    {
        uint64_t vaddr;
        CapPtr cap;
        uint8_t level; // counted 0,1,2,3 for PML1 to PML4

        uint64_t size() const { return 1<<(12+9*level); }
        bool contains(uint64_t ptr) const {
            return (vaddr <= ptr) && (ptr < vaddr+size());
        }
    };

    MemMapper(CapAlloc* caps, CapPtr kmem)
        : caps(caps), kmem(kmem)
    { }

    /** call once to create the initial root map at the given
     *  address in the capspace. */
    optional<void> installPML4(CapPtr dstCap);

    /** map a frame and create missing tables on demand. */
    optional<void> mmap(uint64_t vaddr, uint64_t length, int prot,
        CapPtr frame, uint64_t offset);

    /** helps to allocate a frame. */
    optional<CapPtr> createFrame(size_t size);

    /** helps to create a page map. */
    optional<CapEntry*> createPageMap(CapPtr dstCap, int level);

protected:
    CapAlloc* caps;
    CapPtr kmem;
    VectorMax<MapInfo,MAX_PAGE_MAPS> pagemaps;
};

} // namespace mythos
