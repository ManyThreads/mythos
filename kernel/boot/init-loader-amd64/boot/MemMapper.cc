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

#include "util/alignments.hh"
#include <boot/MemMapper.hh>

namespace mythos {


optional<CapPtr> MemMapper::createFrame(size_t size)
{
    auto kmemEntry = caps->get(kmemCap);
    TypedCap<IAllocator> kmem(kmemEntry);
    if (!kmem) RETHROW(kmem);

    CapPtr frameCap = caps->alloc();
    auto frameEntry = caps->get(frameCap);
    if (!frameEntry) RETHROW(frameEntry);

    auto frame = MemoryRegionFactory::factory(
        *frameEntry, *kmemEntry, kmemCap, *kmem, size, 1ull<<(12+9));
    if (!frame) RETHROW(frame);

    RETURN(frameCap);
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
        *dstEntry, *kmemEntry, kmemCap, *kmem, level+1); // PageMap counts 1,2,3,4
    if (!pagemap) {
        dstEntry->reset();
        RETHROW(pagemap);
    }
    RETURN(*dstEntry);
}


optional<void> MemMapper::installPML4(CapPtr dstCap)
{
    auto res = createPageMap(dstCap, 3);
    if (!res) RETHROW(res);
    pagemaps.push_back({0, dstCap, 3});
    RETURN(Error::SUCCESS);
}


optional<void> MemMapper::mmap(
    uint64_t vaddr, uint64_t length, bool writable, bool executable,
    CapPtr frameCap, uint64_t offset)
{
    auto remaining = length;
    auto curAddr = vaddr;
    auto curOffset = offset;

    // 0) get info about frame and check for plausibility
    TypedCap<IFrame> frame(caps->get(frameCap));
    if (!frame) RETHROW(frame);
    auto frameInfo = frame.getFrameInfo();
    if (curOffset + remaining > frameInfo.size)
        THROW(Error::INSUFFICIENT_RESOURCES);
    if (!Align4K.is_aligned(remaining) ||
        !Align4K.is_aligned(curAddr) ||
        !Align4K.is_aligned(curOffset))
        THROW(Error::UNALIGNED);
    auto flags = protocol::PageMap::MapFlags()
        .writable(writable && frameInfo.writable)
        .executable(executable);

    // continue on the next page map until all of the frame is mapped
    while (remaining > 0) {
        bool isAligned2M =
            Align2M.is_aligned(frameInfo.start.physint()+curOffset) &&
            remaining > (1ull << 21) &&
            Align2M.is_aligned(curAddr);

        // 1) find page maps that contain vaddr
        // there can be only one per level
        MapInfo* levels[] = {nullptr, nullptr, nullptr, nullptr};
        foreach (auto& entry : pagemaps) {
            if (entry.contains(curAddr)) levels[entry.level] = &entry;
        }

        // 2) add missing page maps
        for (int l=3; l>=0; l--) {
            if (levels[l]) continue; // 2.1) page map is present
            if (l==1 && isAligned2M) break; // 2.2) pml1 not needed

            // 2.2) allocate a page map for this level
            CapPtr pmCap = caps->alloc();
            auto dstEntry = createPageMap(pmCap, l);
            if (!pmEntry) RETHROW(pmEntry);

            // 2.3) install the page map in the previous level
            TypedCap<IPageMap> pmlup(caps->get(levels[l+1].cap));
            if (!pmlup) RETHROW(pmlup);
            auto psize = 1ull << (12+9*l);
            auto idx = (curAddr / psize) % 512;
            auto req = protocol::PageMap::MapFlags()
                .writable(true).configurable(true);
            auto res = pmul->mapTable(*pmEntry, req, idx));

            // 2.4) update levels[l] and pagemaps
            pagemaps.push_back({curAddr/psize*psize, pmCap, l});
            levels[l] = &pagemaps.back();
        }

        // 3) insert frames in lowest page table until end of frame or table
        // have to use PML1 if present, else PML2 is good because of step (2.2)
        auto level = 0;
        auto psize = 1ull << 12; // 4KiB
        if (!levels[0]) {
            level = 1;
            psize = (1ull << 21); // 2MiB
        }
        TypedCap<IPageMap> pagemap(caps->get(levels[level]));
        if (!pagemap) RETHROW(pagemap);
        auto idx = (curAddr / psize) % 512;
        while (idx < 512 && remaining >= psize) {
            auto res = pagemap->mapFrame(idx, caps.get(frameCap),
                flags, curOffset);
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
