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
  extern KernelMemory* kmem_root();
  namespace boot {
    using namespace mythos::init;

HookRegistry<InitLoader> initLoaderEvent;
InitLoaderPlugin initloaderplugin;

extern char app_image_start SYMBOL("app_image_start");

/// @todo The load_init could be integrated as plugin
optional<void> load_init()
{
  RETURN(InitLoader(&app_image_start).load());
}

InitLoader::InitLoader(char* image)
  : _img(image)
  , _caps(CAP_ALLOC_END-CAP_ALLOC_START)
  , _frames(0)
  , _maxFrames(0)
  , _mem(kmem_root())
  , _memEntry(kmem_root_entry())
{}

InitLoader::~InitLoader() {}

optional<void> InitLoader::load()
{
  if (!_img.isValid()) RETURN(Error::GENERIC_ERROR);
  MLOG_INFO(mlog::boot, "found init application image at", (void*)&app_image_start);

  // order matters here
  optional<void> res(Error::SUCCESS);
  if (res) res = initCSpace();
  if (res) res = createDirectories();
  if (res) res = createMemoryRegion();
  if (res) res = createMsgFrame();
  if (res) res = loadImage();

  // configure and schedule the execution context, should be the last step
  if (res) res = createEC();
  RETURN(res);
}

size_t InitLoader::countPages()
{
  size_t count = 0;
  for (size_t i=0; i <_img.phnum(); ++i) {
    auto ph = _img.phdr(i);
    if (ph->type == elf64::PT_LOAD) {
      count += toPageRange(ph).getSize();
    }
  }
  return count;
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
optional<Object*> InitLoader::create(CapEntry* dstEntry, ARGS const&...args)
{
  if (!dstEntry->acquire()) THROW(Error::LOST_RACE);
  auto res = Factory::factory(dstEntry, _memEntry, _memEntry->cap(), _mem, args...);
  if (!res) dstEntry->reset();
  RETURN(res);
}

optional<void> InitLoader::csSet(CapPtr dst, CapEntry& root)
{
  ASSERT(root.cap().isUsable());
  auto& dstEntry = *_cspace->get(dst);
  auto res = dstEntry.acquire();
  if (!res) RETHROW(res);
  RETURN(cap::inherit(root, root.cap(), dstEntry, root.cap().asReference()));
}

optional<void> InitLoader::csSet(CapPtr dst, IKernelObject& obj)
{
  auto& dstEntry = *_cspace->get(dst);
  auto res = dstEntry.acquire();
  if (!res) RETHROW(res);
  RETURN(cap::inherit(*_memEntry, _memEntry->cap(), dstEntry, Cap(image2kernel(&obj))));
}

optional<void> InitLoader::initCSpace()
{
  MLOG_INFO(mlog::boot, "create initial cspace ...");
  ASSERT(CapMap::cap_count(12) == CSpaceLayout::SIZE);
  auto ocspace = CapMapFactory::initial(_memEntry, _memEntry->cap(), _mem,
      CapPtrDepth(12), CapPtrDepth(20), CapPtr(0));
  if (!ocspace) RETHROW(ocspace);
  _cspace = *ocspace;

  initLoaderEvent.trigger_before(*this);

  MLOG_INFO(mlog::boot, "... create cspace reference in cap", CSPACE);
  auto res = csSet(CSPACE, _cspace->getRoot());
  if (!res) RETHROW(res);

  MLOG_INFO(mlog::boot, "... create KM reference in cap", KM);
  res = csSet(KM, *_memEntry);
  if (!res) RETHROW(res);

  MLOG_INFO(mlog::boot, "... create portal in cap", PORTAL);
  auto portal = create<Portal,PortalFactory>(_cspace->get(PORTAL));
  if (!portal) RETHROW(portal);
  _portal = *portal;

  MLOG_INFO(mlog::boot, "... create example factory in cap", EXAMPLE_FACTORY);
  res = optional<void>(Error::SUCCESS);
  if (res) res = csSet(EXAMPLE_FACTORY, factory::example);
  if (res) res = csSet(MEMORY_REGION_FACTORY, factory::memoryRegion);
  if (res) res = csSet(EXECUTION_CONTEXT_FACTORY, factory::executionContext);
  if (res) res = csSet(PORTAL_FACTORY, factory::portal);
  if (res) res = csSet(CAPMAP_FACTORY, factory::capmap);
  if (res) res = csSet(PAGEMAP_FACTORY, factory::pagemap);
  if (res) res = csSet(UNTYPED_MEMORY_FACTORY, factory::untypedMemory);
  if (!res) RETHROW(res);

  MLOG_INFO(mlog::boot, "... create memory regions root in cap", DEVICE_MEM);
  {
    auto res = csSet(CapPtr(DEVICE_MEM), boot::device_memory_root_entry());
    if (!res) RETHROW(res);
  }

  ASSERT(cpu::getNumThreads() <= SCHEDULERS_START-APP_CAP_START);
  MLOG_INFO(mlog::boot, "... create scheduling context caps in caps", SCHEDULERS_START, "till", SCHEDULERS_START+cpu::getNumThreads()-1);
  for (cpu::ThreadID id = 0; id < cpu::getNumThreads(); ++id) {
    auto res = csSet(SCHEDULERS_START+id, boot::getScheduler(id));
    if (!res) RETHROW(res);
  }

  ASSERT(cpu::getNumThreads() <= INTERRUPT_CONTROL_START-APP_CAP_START);
  MLOG_INFO(mlog::boot, "... create interrupt controller caps in caps", INTERRUPT_CONTROL_START, "till", INTERRUPT_CONTROL_START+cpu::getNumThreads()-1);
  for (cpu::ThreadID id = 0; id < cpu::getNumThreads(); ++id) {
    auto res = csSet(INTERRUPT_CONTROL_START+id, boot::getInterruptController(id));
    if (!res) RETHROW(res);
  }

  initLoaderEvent.trigger_after(*this);

  RETURN(Error::SUCCESS);
}

optional<void> InitLoader::createDirectories()
{
  MLOG_INFO(mlog::boot, "create page mapping structures ...");
  // we assume the mappings are only in the first 1 GB of the logical mem
  // we also use 2MiB pages ... so we need only one of the PLM2-4
  auto pml4 = create<PageMap, PageMapFactory>(_cspace->get(PML4), 4u);
  if (!pml4) RETHROW(pml4);
  auto pml3 = create<PageMap, PageMapFactory>(_cspace->get(PML3), 3u);
  if (!pml3) RETHROW(pml3);
  auto pml2 = create<PageMap, PageMapFactory>(_cspace->get(PML2), 2u);
  if (!pml2) RETHROW(pml2);
  auto res = mapDirectory(PML4, PML3, 0);
  if (!res) RETHROW(res);
  res = mapDirectory(PML3, PML2, 0);
  RETURN(res);
}

optional<void> InitLoader::mapDirectory(size_t target, size_t entry, size_t index)
{
  TypedCap<IPageMap> pm(_cspace->get(target));
  auto tableEntry = _cspace->get(entry);
  if (!tableEntry) THROW(Error::INVALID_CAPABILITY);
  auto req = protocol::PageMap::MapFlags().writable(true).configurable(true);
  if (!pm) RETHROW(pm);
  RETURN(pm->mapTable(tableEntry, req, index));
}

optional<void> InitLoader::createMemoryRegion()
{
  _maxFrames = countPages();
  MLOG_INFO(mlog::boot, "create dynamic memory region with", _maxFrames, "+1 frames ...");
  _maxFrames++; // one message buffer frame
  auto regionEntry = _cspace->get(DYNAMIC_REGION);
  auto region = create<MemoryRegion, MemoryRegionFactory>
    (regionEntry, _maxFrames*PageAlign::alignment(), PageAlign::alignment());
  if (!region) RETHROW(region);
  _regionStart = region->getFrameInfo(regionEntry->cap()).start;
  RETURN(Error::SUCCESS);
}

optional<size_t> InitLoader::mapFrame(size_t pageNum, bool writable, bool executable)
{
  auto frameNum = allocFrame();
  auto memEntry = _cspace->get(DYNAMIC_REGION);
  if (!memEntry) THROW(Error::INVALID_CAPABILITY);

  TypedCap<IPageMap> pml2(_cspace->get(PML2));
  if (!pml2) RETHROW(pml2);

  auto res = pml2->mapFrame(pageNum, memEntry, 
                            protocol::PageMap::MapFlags().writable(writable).executable(executable), 
                            frameNum*PageAlign::alignment());
  if (!res) RETHROW(res);
  return frameNum;
}

optional<void> InitLoader::createMsgFrame()
{
  const auto page = 10;
  ipc_vaddr = page << 21;
  MLOG_INFO(mlog::boot, "map message buffer frame to page", page);

  auto frameNum = mapFrame(page, true, false);
  if (!frameNum) return frameNum;

  auto memEntry = _cspace->get(DYNAMIC_REGION);
  auto memCap = memEntry->cap();

  typedef protocol::Frame::FrameReq FrameReq;
  // we insert 2MiB pages, which have the size of 512*4KiB
  auto request = FrameReq().offset((*frameNum)*512).size(4096).writable(true).device(false);
  auto frameEntry = _cspace->get(MSG_FRAME);
  auto res = cap::derive(*memEntry, *frameEntry, memCap, request);
  if (!res) RETHROW(res);
  RETURN(_portal->setInvocationBuf(frameEntry, 0));
}

optional<void> InitLoader::loadImage()
{
  for (size_t i=0; i <_img.phnum(); ++i) {
    auto ph = _img.phdr(i);
    if (ph->type == elf64::PT_LOAD) {
      auto res = load(ph);
      if (!res) RETHROW(res);
    }
  }
  RETURN(Error::SUCCESS);
}

optional<void> InitLoader::createEC()
{
  MLOG_INFO(mlog::boot, "create and initialize EC");
  auto ec = create<ExecutionContext,ExecutionContextFactory>(_cspace->get(EC));
  if (!ec) RETHROW(ec);
  _portal->setOwner(_cspace->get(EC));
  optional<void> res(Error::SUCCESS);
  if (res) res = ec->setCapSpace(_cspace->get(CSPACE));
  if (res) res = ec->setAddressSpace(_cspace->get(PML4));
  if (res) res = ec->setSchedulingContext(_cspace->get(SCHEDULERS_START));
  if (!res) RETHROW(res);
  ec->getThreadState().rdi = ipc_vaddr;
  ec->setEntryPoint(_img.header()->entry);
  ec->setTrapped(false);
  RETURN(Error::SUCCESS);
}

Range<uintptr_t> InitLoader::toFrameRange(const elf64::PHeader* ph)
{
  auto start = PhysPtr<char>::fromImage(&app_image_start +ph->offset).physint();
  auto end = PhysPtr<char>::fromImage(&app_image_start +ph->offset +ph->filesize).physint();
  start = PageAlign::round_down(start)/PageAlign::alignment();
  end = PageAlign::round_up(end)/PageAlign::alignment();
  return {start, end};
}

Range<uintptr_t> InitLoader::toPageRange(const elf64::PHeader* ph)
{
  auto start = PageAlign::round_down(ph->vaddr)/PageAlign::alignment();
  auto end = PageAlign::round_up(ph->vaddr + ph->memsize)/PageAlign::alignment();
  return {start, end};
}

optional<void> InitLoader::load(const elf64::PHeader* ph)
{
  MLOG_INFO(mlog::boot, "\tload", DVAR(ph->type), DVAR(ph->flags), DVARhex(ph->offset),
        DVARhex(ph->vaddr), DVARhex(ph->filesize), DVARhex(ph->memsize),
        DVARhex(ph->alignment));
  ASSERT(ph->alignment <= PageAlign::alignment());

  auto pageRange = toPageRange(ph);
  if (pageRange.getSize() == 0) RETURN(Error::SUCCESS); // nothing to do

  auto page = pageRange.getStart();
  auto frameNum = mapFrame(page, ph->flags&elf64::PF_W, ph->flags&elf64::PF_X);
  if (!frameNum) return frameNum;
  size_t firstFrameIndex = *frameNum;

  for (size_t i = 1; i < pageRange.getSize(); ++i) {
    auto page = pageRange.getStart()+i;
    auto frameNum = mapFrame(page, ph->flags&elf64::PF_W, ph->flags&elf64::PF_X);
    if (!frameNum) return frameNum;
  }

  // zero the pages
  auto start = _regionStart.plusbytes(firstFrameIndex*PageAlign::alignment());
  memset(start.log(), 0, pageRange.getSize()*PageAlign::alignment());
  // copy the data
  start.incbytes(ph->vaddr % PageAlign::alignment());
  memcpy(start.log(), &app_image_start+ph->offset, ph->filesize);
  RETURN(Error::SUCCESS);
}

size_t InitLoader::allocFrame()
{
  ASSERT(_frames < _maxFrames);
  return _frames++;
}

} // namespace boot
} // namespace mythos
