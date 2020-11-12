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

#include "util/align.hh"
#include "objects/DeviceMemory.hh"
#include "objects/MemoryRegion.hh"
#include "objects/PageMapAmd64.hh"
#include <boot/MemMapper.hh>
#include "mythos/init.hh"
#include "boot/mlog.hh"

namespace mythos {


optional<CapPtr> MemMapper::createFrame(CapPtr frameCap, size_t size, size_t alignment)
{
    auto kmemEntry = caps->get(kmemCap);
    TypedCap<IAllocator> kmem(kmemEntry);
    if (!kmem) RETHROW(kmem);

    auto frameEntry = caps->get(frameCap);
    if (!frameEntry) RETHROW(frameEntry);
    if (!frameEntry->acquire()) THROW(Error::LOST_RACE);

    auto frame = MemoryRegionFactory::factory(
        *frameEntry, *kmemEntry, kmem.cap(), *kmem, size, alignment);
    if (!frame) {
        frameEntry->reset();
        RETHROW(frame);
    }

    return frameCap;
}

optional<CapEntry*> MemMapper::createPageMap(CapPtr dstCap, int level)
{
    auto kmemEntry = caps->get(kmemCap);
    TypedCap<IAllocator> kmem(kmemEntry);
    if (!kmem) RETHROW(kmem);

    auto dstEntry = caps->get(dstCap);
    if (!dstEntry) RETHROW(dstEntry);
    if (!dstEntry->acquire()) THROW(Error::LOST_RACE);

    auto pagemap = PageMapFactory::factory(
        *dstEntry, *kmemEntry, kmem.cap(), *kmem, level+1); // PageMap counts 1,2,3,4
    if (!pagemap) {
        dstEntry->reset();
        RETHROW(pagemap);
    }

    return dstEntry;
}


optional<void> MemMapper::installPML4(CapPtr dstCap)
{
    auto res = createPageMap(dstCap, 3);
    if (!res) RETHROW(res);
    pagemaps.push_back({0, dstCap, 3});
    RETURN(Error::SUCCESS);
}

optional<void> MemMapper::mmapDevice(
    uintptr_t vaddr, size_t length,
    bool writable, bool executable,
    uintptr_t physaddr)
{
    // TODO this should be configurable instead of importing mythos/init.hh
    auto memEntry = caps->get(init::DEVICE_MEM);
    TypedCap<DeviceMemory> mem(memEntry);
    if (!mem) RETHROW(mem);
    auto dstPtr = caps->alloc();
    if (!dstPtr) RETHROW(dstPtr);
    auto dstEntry = caps->get(*dstPtr);
    if (!dstEntry) RETHROW(dstEntry);
    auto res = mem->deriveFrame(**memEntry, memEntry->cap(), **dstEntry, physaddr, length, false);
    if (!res) RETHROW(res);
    RETURN(mmap(vaddr, length, writable, executable, *dstPtr, 0)); // offset 0 of the derived frame
}

optional<void> MemMapper::mmap(
    uintptr_t vaddr, size_t length, bool writable, bool executable,
    CapPtr frameCap, size_t offset)
{
    auto remaining = length;
    auto curAddr = vaddr;
    auto curOffset = offset;
    MLOG_INFO(mlog::boot, "    MemMapper::mmap", DVARhex(vaddr),
        DVARhex(length), DVAR(writable), DVAR(executable),
        DVAR(frameCap), DVARhex(offset));

    // 0) get info about frame and check for plausibility
    auto frameEntry = caps->get(frameCap);
    if (!frameEntry) RETHROW(frameEntry);
    TypedCap<IFrame> frame(frameEntry);
    if (!frame) RETHROW(frame);
    auto frameInfo = frame.getFrameInfo();
    if (curOffset + remaining > frameInfo.size)
        THROW(Error::INSUFFICIENT_RESOURCES);
    if (!is_aligned(remaining, align4K) ||
        !is_aligned(curAddr, align4K) ||
        !is_aligned(curOffset, align4K))
        THROW(Error::UNALIGNED);
    auto flags = protocol::PageMap::MapFlags()
        .writable(writable && frameInfo.writable)
        .executable(executable);

    // continue on the next page map until all of the frame is mapped
    while (remaining > 0) {
        bool isAligned2M =
            is_aligned(frameInfo.start.physint()+curOffset, align2M) &&
            remaining >= align2M &&
            is_aligned(curAddr, align2M);

        // 1) find page maps that contain vaddr
        // there can be only one per level
        MapInfo* levels[] = {nullptr, nullptr, nullptr, nullptr};
        for (auto& entry : pagemaps) {
            if (entry.contains(curAddr)) levels[entry.level] = &entry;
        }

        // 2) add missing page maps
        for (int l=3; l>=0; l--) {
            if (levels[l]) continue; // 2.1) page map is present
            if (l==0 && isAligned2M) break; // 2.2) pml1 not needed

            // 2.2) allocate a page map for this level
            auto pmCap = caps->alloc();
            if (!pmCap) RETHROW(pmCap);
            auto pmEntry = createPageMap(*pmCap, l);
            if (!pmEntry) RETHROW(pmEntry);

            // 2.3) install the page map in the previous level
            TypedCap<IPageMap> pmlup(caps->get(levels[l+1]->cap));
            if (!pmlup) RETHROW(pmlup);
            auto tsize = FrameSize::pageLevel2Size(l+1); // bytes spanned by this page map
            auto idx = (curAddr / tsize) % 512; // position of this map in parent map
            auto req = protocol::PageMap::MapFlags()
                .writable(true).configurable(true);
            auto res = pmlup->mapTable(*pmEntry, req, idx);
            if (!res) RETHROW(res);

            // 2.4) update levels[l] and pagemaps
            pagemaps.push_back({round_down(curAddr, tsize), *pmCap, uint8_t(l)});
            levels[l] = &pagemaps.back();
            MLOG_DETAIL(mlog::boot, "    added page map",
                DVARhex(levels[l]->vaddr), DVARhex(levels[l]->size()),
                DVAR(levels[l]->level), DVAR(levels[l]->cap));
        }

        // 3) insert frames in lowest page table until end of frame or table
        // have to use PML1 if present, else PML2 is good because of step (2.2)
        auto level = 0;
        auto psize = FrameSize::pageLevel2Size(0); // 4KiB
        if (!levels[0]) {
            level = 1;
            psize = FrameSize::pageLevel2Size(1); // 2MiB
        }
        TypedCap<IPageMap> pagemap(caps->get(levels[level]->cap));
        if (!pagemap) RETHROW(pagemap);
        auto idx = (curAddr / psize) % 512; // position of this vaddr in the page map
        while (idx < 512 && remaining >= psize) {
            auto res = pagemap->mapFrame(idx, *frameEntry, flags, curOffset);
            if (!res) RETHROW(res);
            idx++;
            remaining -= psize;
            curOffset += psize;
            curAddr += psize;
        }
    }
    RETURN(Error::SUCCESS);
}

} // namespace mythos
