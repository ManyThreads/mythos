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

#include "boot/load_init.hh"

#include "util/elf64.hh"
#include "util/align.hh"
#include "util/compiler.hh"
#include "util/mstring.hh"
#include "util/assert.hh"
#include "mythos/init.hh"
#include "objects/ops.hh"
#include "objects/CapMap.hh"
#include "objects/KernelMemory.hh"
#include "objects/MemoryRegion.hh"
#include "objects/PageMapAmd64.hh"
#include "objects/ExecutionContext.hh"
#include "objects/SchedulingContext.hh"
#include "objects/Portal.hh"
#include "objects/Example.hh"
#include "boot/mlog.hh"
#include "boot/memory-root.hh"
#include "boot/DeployHWThread.hh"


namespace mythos {
    
Event<boot::InitLoader&> event::initLoader;
    
namespace boot {

InitLoader::InitLoader(char* image)
  : _img(image)
  , _mem(kmem_root())
  , _memEntry(kmem_root_entry())
  , capAlloc(init::CAP_ALLOC_START,
        init::CAP_ALLOC_END-init::CAP_ALLOC_START)
  , memMapper(&capAlloc, mythos::init::KM)
{
  MLOG_INFO(mlog::boot, "found init application image at", (void*)image);
}

InitLoader::~InitLoader() { }

optional<void> InitLoader::load()
{
  if (!_img.isValid()) RETURN(Error::GENERIC_ERROR);

  // order matters here
  optional<void> res(Error::SUCCESS);
  if (res) res = initCSpace();
  auto ipc_vaddr = loadImage();
  res = ipc_vaddr;
  if (res) MLOG_INFO(mlog::boot, "init invokation buffer", DVARhex(*ipc_vaddr));
  if (res) res = createPortal(*ipc_vaddr, init::PORTAL);
  if (res) res = createEC(*ipc_vaddr);
  RETURN(res);
}

namespace factory {
  ExampleFactory example;
  MemoryRegionFactory memoryRegion;
  ExecutionContextFactory executionContext;
  PortalFactory portal;
  CapMapFactory capmap;
  PageMapFactory pagemap;
  KernelMemoryFactory untypedMemory;
} // namespace example

template<class Object, class Factory, class... ARGS>
optional<Object*> InitLoader::create(optional<CapEntry*> dstEntry, ARGS const&...args)
{
  if (!dstEntry) RETHROW(dstEntry);
  if (!dstEntry->acquire()) THROW(Error::LOST_RACE);
  auto res = Factory::factory(*dstEntry, _memEntry, _memEntry->cap(), _mem, args...);
  if (!res) dstEntry->reset();
  RETURN(res);
}

optional<void> InitLoader::csSet(CapPtr dst, CapEntry& root)
{
  ASSERT(root.cap().isUsable());
  auto dstEntry = capAlloc.get(dst);
  if (!dstEntry) RETHROW(dstEntry);
  auto res = dstEntry->acquire();
  if (!res) RETHROW(res);
  RETURN(cap::inherit(root, root.cap(), **dstEntry, root.cap().asReference()));
}

optional<void> InitLoader::csSet(CapPtr dst, IKernelObject& obj)
{
  auto dstEntry = capAlloc.get(dst);
  if (!dstEntry) RETHROW(dstEntry);
  auto res = dstEntry->acquire();
  if (!res) RETHROW(res);
  RETURN(cap::inherit(*_memEntry, _memEntry->cap(), **dstEntry, Cap(image2kernel(&obj))));
}

optional<void> InitLoader::initCSpace()
{
  MLOG_INFO(mlog::boot, "create initial cspace ...");
  ASSERT(CapMap::cap_count(12) == init::CSpaceLayout::SIZE);
  auto ocspace = CapMapFactory::initial(_memEntry, _memEntry->cap(), _mem,
      CapPtrDepth(12), CapPtrDepth(20), CapPtr(0));
  if (!ocspace) RETHROW(ocspace);
  capAlloc.setCSpace(*ocspace);

  MLOG_INFO(mlog::boot, "... create cspace reference in cap", init::CSPACE);
  auto res = csSet(init::CSPACE, ocspace->getRoot());
  if (!res) RETHROW(res);

  MLOG_INFO(mlog::boot, "... create KM reference in cap", init::KM);
  res = csSet(init::KM, *_memEntry);
  if (!res) RETHROW(res);

  MLOG_INFO(mlog::boot, "... create address space in cap", init::PML4);
  res = memMapper.installPML4(init::PML4);
  if (!res) RETHROW(res);

  MLOG_INFO(mlog::boot, "... create example factory in cap", init::EXAMPLE_FACTORY);
  res = optional<void>(Error::SUCCESS);
  if (res) res = csSet(init::EXAMPLE_FACTORY, factory::example);
  if (res) res = csSet(init::MEMORY_REGION_FACTORY, factory::memoryRegion);
  if (res) res = csSet(init::EXECUTION_CONTEXT_FACTORY, factory::executionContext);
  if (res) res = csSet(init::PORTAL_FACTORY, factory::portal);
  if (res) res = csSet(init::CAPMAP_FACTORY, factory::capmap);
  if (res) res = csSet(init::PAGEMAP_FACTORY, factory::pagemap);
  if (res) res = csSet(init::UNTYPED_MEMORY_FACTORY, factory::untypedMemory);
  if (!res) RETHROW(res);

  MLOG_INFO(mlog::boot, "... create memory regions root in cap", init::DEVICE_MEM);
  {
    auto res = csSet(init::DEVICE_MEM, boot::device_memory_root_entry());
    if (!res) RETHROW(res);
  }

  ASSERT(cpu::getNumThreads() <= init::SCHEDULERS_START - init::APP_CAP_START);
  MLOG_INFO(mlog::boot, "... create scheduling context caps in caps",
        init::SCHEDULERS_START, "till", init::SCHEDULERS_START+cpu::getNumThreads()-1);
  for (cpu::ThreadID id = 0; id < cpu::getNumThreads(); ++id) {
    auto res = csSet(init::SCHEDULERS_START+id, boot::getScheduler(id));
    if (!res) RETHROW(res);
  }

  ASSERT(cpu::getNumThreads() <= init::INTERRUPT_CONTROL_START - init::APP_CAP_START);
  MLOG_INFO(mlog::boot, "... create interrupt controller caps in caps",
        init::INTERRUPT_CONTROL_START, "till", init::INTERRUPT_CONTROL_START+cpu::getNumThreads()-1);
  for (cpu::ThreadID id = 0; id < cpu::getNumThreads(); ++id) {
    auto res = csSet(init::INTERRUPT_CONTROL_START+id, boot::getInterruptController(id));
    if (!res) RETHROW(res);
  }

  event::initLoader.emit(*this);

  RETURN(Error::SUCCESS);
}


optional<void> InitLoader::createPortal(uintptr_t ipc_vaddr, CapPtr dstPortal)
{
    auto size = 2*1024*1024;
    MLOG_INFO(mlog::boot, "... create portal in cap", dstPortal);
    auto portal = create<Portal,PortalFactory>(capAlloc.get(dstPortal));
    if (!portal) RETHROW(portal);

    auto frameCap = memMapper.createFrame(size, 2*1024*1024);
    if (!frameCap) RETHROW(frameCap);
    MLOG_INFO(mlog::boot, "... map invocation buffer",
        DVAR(*frameCap), DVARhex(ipc_vaddr));
    memMapper.mmap(ipc_vaddr, size, true, false, *frameCap, 0);
    auto res = portal->setInvocationBuf(capAlloc.get(*frameCap), 0);
    if (!res) RETHROW(res);

    _portal = *portal; // TODO return this?
    RETURN(Error::SUCCESS);
}

optional<uintptr_t> InitLoader::loadImage()
{
    uintptr_t ipc_addr = round_up(1u, align2M);
    // 1) figure out how much bytes we need
    size_t size = 0;
    for (size_t i=0; i <_img.phnum(); ++i) {
        auto ph = _img.phdr(i);
        if (ph->type == elf64::PT_LOAD) {
            auto begin = round_down(ph->vaddr, align2M);
            auto end = round_up(ph->vaddr + ph->memsize, align2M);
            size += end-begin;
            if (end >= ipc_addr) {
              ipc_addr = round_up(end + 1u, align2M);
            }
        }
    }

    // 2) allocate a frame
    auto frameCap = memMapper.createFrame(size, 2*1024*1024);
    if (!frameCap) RETHROW(frameCap);
    MLOG_INFO(mlog::boot, "... allocated frame for application image",
        DVAR(*frameCap), DVAR(size));

    // 3) process each program header: map to page, copy contents
    size_t offset = 0;
    for (size_t i=0; i <_img.phnum(); ++i) {
        auto ph = _img.phdr(i);
        if (ph->type == elf64::PT_LOAD) {
            auto begin = round_down(ph->vaddr, align2M);
            auto end = round_up(ph->vaddr + ph->memsize, align2M);
            auto res = loadProgramHeader(ph, *frameCap, offset);
            if (!res) RETHROW(res);
            offset += end-begin;
        }
    }
    return ipc_addr;
}

optional<void> InitLoader::loadProgramHeader(
    const elf64::PHeader* ph, CapPtr frameCap, size_t offset)
{
    MLOG_INFO(mlog::boot, "... load PH", DVAR(ph->type), DVAR(ph->flags), DVARhex(ph->offset),
        DVARhex(ph->vaddr), DVARhex(ph->filesize), DVARhex(ph->memsize),
        DVARhex(ph->alignment), DVARhex(offset));
    ASSERT(ph->alignment <= align2M);

    // align the page in the logical address space
    auto vbegin = round_down(ph->vaddr, ph->alignment);
    auto vend = round_up(ph->vaddr+ph->memsize,ph->alignment);
    if (vend-vbegin == 0) RETURN(Error::SUCCESS); // nothing to do

    // map the frame into the pages
    memMapper.mmap(vbegin, vend-vbegin,
        ph->flags&elf64::PF_W, ph->flags&elf64::PF_X,
        frameCap, offset);

    // get frame info for its address in the kernel memory area
    TypedCap<IFrame> frame(capAlloc.get(frameCap));
    if (!frame) RETHROW(frame);
    auto frameInfo = frame.getFrameInfo();

    // zero the pages, then copy the data
    auto pstart = frameInfo.start.plusbytes(offset);
    //MLOG_INFO(mlog::boot, "    zeroing", DVARhex(pstart), DVARhex(vend-vbegin));
    memset(pstart.log(), 0, vend-vbegin);
    pstart.incbytes(ph->vaddr - vbegin);
    //MLOG_INFO(mlog::boot, "    zeroing2", DVARhex(pstart), DVARhex(vend-vbegin));
    memcpy(pstart.log(), _img.getData(*ph), ph->filesize);
    //MLOG_INFO(mlog::boot, "    zeroing3", DVARhex(pstart), DVARhex(vend-vbegin));
    RETURN(Error::SUCCESS);
}


optional<void> InitLoader::createEC(uintptr_t ipc_vaddr)
{
  MLOG_INFO(mlog::boot, "create and initialize EC");
  auto ec = create<ExecutionContext,ExecutionContextFactory>(capAlloc.get(init::EC));
  if (!ec) RETHROW(ec);
  _portal->setOwner(capAlloc.get(init::EC));
  optional<void> res(Error::SUCCESS);
  if (res) res = ec->setCapSpace(capAlloc.get(init::CSPACE));
  if (res) res = ec->setAddressSpace(capAlloc.get(init::PML4));
  if (res) res = ec->setSchedulingContext(capAlloc.get(init::SCHEDULERS_START));
  if (!res) RETHROW(res);
  ec->getThreadState().rdi = ipc_vaddr;
  ec->setEntryPoint(_img.header()->entry);
  ec->setTrapped(false);
  RETURN(Error::SUCCESS);
}


} // namespace boot
} // namespace mythos
