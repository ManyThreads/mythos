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
        uintptr_t vaddr;
        CapPtr cap;
        uint8_t level; // counted 0,1,2,3 for PML1 to PML4

        /** size of this page map */
        size_t size() const { return FrameSize::pageLevel2Size(level+1); }
        bool contains(uintptr_t ptr) const {
            return (vaddr <= ptr) && (ptr < vaddr+size());
        }
    };

    MemMapper(CapAlloc* caps, CapPtr kmemCap)
        : caps(caps), kmemCap(kmemCap)
    { }

    /** call once to create the initial root map at the given
     *  address in the capspace. */
    optional<void> installPML4(CapPtr dstCap);

    /** map a frame and create missing tables on demand. */
    optional<void> mmap(
        uintptr_t vaddr, size_t length,
        bool writable, bool executable,
        CapPtr frameCap, size_t offset);

    /** allocate a frame and register it at the specified capability pointer */
    optional<CapPtr> createFrame(CapPtr frameCap, size_t size, size_t alignment);
    optional<CapPtr> createFrame(size_t size, size_t alignment)
    {
        auto frameCap = caps->alloc();
        if (!frameCap) RETHROW(frameCap);
        return createFrame(*frameCap, size, alignment);
    }

    /** allocate a page map of the specified level and register it
     *  in the cspace at the specified capability pointer. */
    optional<CapEntry*> createPageMap(CapPtr dstCap, int level);

protected:
    CapAlloc* caps;
    CapPtr kmemCap;
    VectorMax<MapInfo,MAX_PAGE_MAPS> pagemaps;
};

} // namespace mythos
