#pragma once

#include "mythos/init.hh"
#include "util/assert.hh"
#include "mythos/caps.hh"
#include "runtime/mlog.hh"
#include "util/optional.hh"
#include "util/elf64.hh"
#include "util/align.hh"
#include "runtime/CapAlloc.hh"

extern mythos::CapMap myCS;
extern mythos::PageMap myAS;
extern mythos::KernelMemory kmem;

using namespace mythos;

class Process{
  public:
  Process(char* image)
    : cs(capAlloc())
    , pCapAlloc(cs)
    , img(image)
  {}
  
  optional<void> loadProgramHeader(PortalLock& pl,
      const elf64::PHeader* ph, uintptr_t tmp_vaddr, Frame& f, size_t offset, PageMap& pm)
  {
      MLOG_DETAIL(mlog::app, "... load program header", DVAR(ph->type), DVAR(ph->flags), DVARhex(ph->offset),
          DVARhex(ph->vaddr), DVARhex(ph->filesize), DVARhex(ph->memsize),
          DVARhex(ph->alignment), DVARhex(offset), DVARhex(tmp_vaddr));
      ASSERT(ph->alignment <= align2M);

      // align the page in the logical address space
      auto vbegin = round_down(ph->vaddr, ph->alignment);
      auto vend = round_up(ph->vaddr+ph->memsize,ph->alignment);
      if (vend-vbegin == 0) RETURN(Error::SUCCESS); // nothing to do

      // map the frame into the pages
      protocol::PageMap::MapFlags mf = 0;
      mf.writable = ph->flags&elf64::PF_W;
      mf.executable = ph->flags&elf64::PF_X;
      MLOG_DETAIL(mlog::app, "... mmap program header", DVARhex(vbegin), DVARhex(vend), DVARhex(offset));
      auto res = pm.mmap(pl, f, vbegin, vend-vbegin, mf, offset);
      TEST(res);

      // zero the pages, then copy the data
      MLOG_DETAIL(mlog::app, "    zeroing", DVARhex(tmp_vaddr + offset), DVARhex(vend-vbegin));
      memset(reinterpret_cast<void*>(tmp_vaddr + offset), 0, vend-vbegin);
      MLOG_DETAIL(mlog::app, "    copy data");
      mythos::memcpy(reinterpret_cast<void*>(tmp_vaddr + offset + ph->vaddr - vbegin), img.getData(*ph), ph->filesize);
      RETURN(Error::SUCCESS);
  }

  optional<uintptr_t> loadImage(PortalLock& pl, PageMap& pm)
  {
      MLOG_DETAIL(mlog::app, "load image ...");
      ASSERT(img.isValid());

      uintptr_t ipc_addr = round_up(1u, align2M);
      // 1) figure out how much bytes we need
      size_t size = 0;
      for (size_t i=0; i <img.phnum(); ++i) {
          auto ph = img.phdr(i);
          if (ph->type == elf64::PT_LOAD) {
              auto begin = round_down(ph->vaddr, align2M);
              auto end = round_up(ph->vaddr + ph->memsize, align2M);
              size += end-begin;
              if (end >= ipc_addr) {
                ipc_addr = round_up(end + 1u, align2M);
              }
          }
      }
      MLOG_DETAIL(mlog::app, "   bytes needed for loading image: ", DVARhex(size));

      // 2) allocate a frame
      MLOG_DETAIL(mlog::app, "allocate frame for application image ...")
      Frame f(capAlloc());
      auto res = f.create(pl, kmem, 2*size, align2M).wait();
      
      uintptr_t tmp_vaddr = 42*align512G;
      MLOG_DETAIL(mlog::app, "temporary map frame to own address space ...", DVARhex(tmp_vaddr));
      MLOG_DETAIL(mlog::app, "   create PageMap");
      PageMap pm3(capAlloc());
      res = pm3.create(pl, kmem, 3).wait();
      TEST(res);
      PageMap pm2(capAlloc());
      res = pm2.create(pl, kmem, 2).wait();
      TEST(res);

      MLOG_DETAIL(mlog::app, "   installMap");
      res = myAS.installMap(pl, pm3, ((tmp_vaddr >> 39) & 0x1FF)<< 39, 4,
        protocol::PageMap::MapFlags().writable(true).configurable(true)).wait();
      TEST(res);
      res = pm3.installMap(pl, pm2, ((tmp_vaddr >> 30) & 0x1FF) << 30, 3,
        protocol::PageMap::MapFlags().writable(true).configurable(true)).wait();
      TEST(res);

      MLOG_DETAIL(mlog::app, "   mmap");
      res = myAS.mmap(pl, f, tmp_vaddr, size, 0x1).wait();
      TEST(res);

      // 3) process each program header: map to page, copy contents
      size_t offset = 0;
      for (size_t i=0; i <img.phnum(); ++i) {
          auto ph = img.phdr(i);
          if (ph->type == elf64::PT_LOAD) {
              auto begin = round_down(ph->vaddr, align2M);
              auto end = round_up(ph->vaddr + ph->memsize, align2M);
              auto res = loadProgramHeader(pl, ph, tmp_vaddr, f, offset, pm);
              if (!res) RETHROW(res);
              offset += end-begin;
          }
      }


      MLOG_DETAIL(mlog::app, "cleaning up ...");
      MLOG_DETAIL(mlog::app, "   unmap frame");
      res = myAS.munmap(pl, tmp_vaddr, size).wait();
      TEST(res);

      MLOG_DETAIL(mlog::app, "   remove maps");
      res = myAS.removeMap(pl, tmp_vaddr, 2).wait();
      TEST(res);
      res = myAS.removeMap(pl, tmp_vaddr, 3).wait();
      TEST(res);

      MLOG_DETAIL(mlog::app, "   move frame");
      res = myCS.move(pl, f.cap(), max_cap_depth, cs.cap(), pCapAlloc(), max_cap_depth).wait();
      TEST(res);
      capAlloc.freeEmpty(f.cap());

      MLOG_DETAIL(mlog::app, "   delete tables");
      capAlloc.free(pm3, pl);
      capAlloc.free(pm2, pl);

      return ipc_addr;
  }

  optional<CapPtr> createProcess(PortalLock& pl){
    MLOG_INFO(mlog::app, __func__);

    /* create CapMap */
    MLOG_DETAIL(mlog::app, "create CapMap ...");
    auto res = cs.create(pl, kmem, CapPtrDepth(12), CapPtrDepth(20), CapPtr(0)).wait();
    TEST(res);
    
    /* copy relevant caps */
    MLOG_DETAIL(mlog::app, "copy relevant Caps ...");
    MLOG_DETAIL(mlog::app, "   kernel memory");
    res = myCS.reference(pl, init::KM, max_cap_depth, cs.cap(), init::KM, max_cap_depth, 0).wait();
    TEST(res);

    MLOG_DETAIL(mlog::app, "   factories");
    for(CapPtr ptr = init::EXAMPLE_FACTORY; ptr <= init::UNTYPED_MEMORY_FACTORY; ptr++){
      res = myCS.reference(pl, ptr, max_cap_depth, cs.cap(), ptr, max_cap_depth, 0).wait();
      TEST(res);
    }
    
    MLOG_DETAIL(mlog::app, "   DEVICE_MEMORY");
    res = myCS.reference(pl, init::DEVICE_MEM, max_cap_depth, cs.cap(), init::DEVICE_MEM, max_cap_depth, 0).wait();
    TEST(res);

    MLOG_DETAIL(mlog::app, "   scheduling contexts");
    //todo: how to check whether cap is existing?
    for(CapPtr ptr = init::SCHEDULERS_START; ptr < init::RAPL_DRIVER_INTEL; ptr++){
      res = myCS.reference(pl, ptr, max_cap_depth, cs.cap(), ptr, max_cap_depth, 0).wait();
      TEST(res);
    }

    MLOG_DETAIL(mlog::app, "   RAPL driver");
    res = myCS.reference(pl, init::RAPL_DRIVER_INTEL, max_cap_depth, cs.cap(), init::RAPL_DRIVER_INTEL, max_cap_depth, 0).wait();
    TEST(res);

    MLOG_DETAIL(mlog::app, "   Interrupt control");
    //todo: how to check whether cap is existing?
    for(CapPtr ptr = init::INTERRUPT_CONTROL_START; ptr < init::INTERRUPT_CONTROL_END; ptr++){
      res = myCS.reference(pl, ptr, max_cap_depth, cs.cap(), ptr, max_cap_depth, 0).wait();
      TEST(res);
    }

    /* create address space */
    //todo: use dynamic table allocation in mmap!
    MLOG_DETAIL(mlog::app, "create PageMaps ...");
    uintptr_t elf_vaddr = 0x400000;

    // create tables
    PageMap pm4(capAlloc());
    auto res_pm = pm4.create(pl, kmem, 4).wait();
    TEST(res_pm);
    PageMap pm3(capAlloc());
    res_pm = pm3.create(pl, kmem, 3).wait();
    TEST(res_pm);
    PageMap pm2(capAlloc());
    res_pm = pm2.create(pl, kmem, 2).wait();
    TEST(res_pm);
    PageMap pm10(capAlloc());
    res_pm = pm10.create(pl, kmem, 1).wait();
    TEST(res_pm);
    PageMap pm11(capAlloc());
    res_pm = pm11.create(pl, kmem, 1).wait();
    TEST(res_pm);
    PageMap pm12(capAlloc());
    res_pm = pm12.create(pl, kmem, 1).wait();
    TEST(res_pm);
    PageMap pm13(capAlloc());
    res_pm = pm13.create(pl, kmem, 1).wait();
    TEST(res_pm);
    PageMap pm14(capAlloc());
    res_pm = pm14.create(pl, kmem, 1).wait();
    TEST(res_pm);
    PageMap pm15(capAlloc());
    res_pm = pm15.create(pl, kmem, 1).wait();
    TEST(res_pm);
    PageMap pm16(capAlloc());
    res_pm = pm16.create(pl, kmem, 1).wait();
    TEST(res_pm);
    PageMap pm17(capAlloc());
    res_pm = pm17.create(pl, kmem, 1).wait();
    TEST(res_pm);

    // install tables
    pm4.installMap(pl, pm3, ((elf_vaddr >> 39) & 0x1FF) << 39, 4,
      mythos::protocol::PageMap::MapFlags().writable(true).configurable(true)).wait();
    pm3.installMap(pl, pm2, ((elf_vaddr >> 30) & 0x1FF) << 30, 3,
      mythos::protocol::PageMap::MapFlags().writable(true).configurable(true)).wait();
    pm2.installMap(pl, pm10, ((elf_vaddr >> 21) & 0x1FF) << 21, 2,
      mythos::protocol::PageMap::MapFlags().writable(true).configurable(true)).wait();
    pm2.installMap(pl, pm11, ((elf_vaddr >> 21) & 0x1FF) + 1 << 21, 2,
      mythos::protocol::PageMap::MapFlags().writable(true).configurable(true)).wait();
    pm2.installMap(pl, pm12, ((elf_vaddr >> 21) & 0x1FF) + 2 << 21, 2,
      mythos::protocol::PageMap::MapFlags().writable(true).configurable(true)).wait();
    pm2.installMap(pl, pm13, ((elf_vaddr >> 21) & 0x1FF) + 3 << 21, 2,
      mythos::protocol::PageMap::MapFlags().writable(true).configurable(true)).wait();
    pm2.installMap(pl, pm14, ((elf_vaddr >> 21) & 0x1FF) + 4 << 21, 2,
      mythos::protocol::PageMap::MapFlags().writable(true).configurable(true)).wait();
    pm2.installMap(pl, pm15, ((elf_vaddr >> 21) & 0x1FF) + 5 << 21, 2,
      mythos::protocol::PageMap::MapFlags().writable(true).configurable(true)).wait();
    pm2.installMap(pl, pm16, ((elf_vaddr >> 21) & 0x1FF) + 6 << 21, 2,
      mythos::protocol::PageMap::MapFlags().writable(true).configurable(true)).wait();
    pm2.installMap(pl, pm17, ((elf_vaddr >> 21) & 0x1FF) + 7 << 21, 2,
      mythos::protocol::PageMap::MapFlags().writable(true).configurable(true)).wait();

    /* load image */
    auto ipc_vaddr = loadImage(pl, pm4);
    TEST(ipc_vaddr);

    /* create EC */
    MLOG_DETAIL(mlog::app, "create EC ...", DVARhex(img.header()->entry));
    ExecutionContext ec(capAlloc());
    res = ec.create(kmem).as(pm4).cs(cs).sched(mythos::init::SCHEDULERS_START + 1)
    .rawFun(reinterpret_cast<int (*)(void*)>(img.header()->entry), reinterpret_cast<void*>(*ipc_vaddr))
    .suspended(true)
    .invokeVia(pl).wait();
    TEST(res);

    /* create portal */
    MLOG_DETAIL(mlog::app, "create Portal ...");
    MLOG_DETAIL(mlog::app, "   map frame at ", DVARhex(*ipc_vaddr));
    Frame fp(capAlloc());
    res = fp.create(pl, kmem, 2*1024*1024, align2M).wait();
    TEST(res);
    res = pm4.mmap(pl, fp, *ipc_vaddr, 2*1024*1024, 0x1).wait();
    TEST(res);

    MLOG_DETAIL(mlog::app, "   create");
    Portal port(capAlloc(), reinterpret_cast<void*>(*ipc_vaddr));
    res = port.create(pl, kmem).wait();
    TEST(res);
    res = port.bind(pl, fp, 0, ec.cap()).wait();
    TEST(res);

    MLOG_DETAIL(mlog::app, "   move Portal");
    res = myCS.move(pl, port.cap(), max_cap_depth, cs.cap(), init::PORTAL, max_cap_depth).wait();
    TEST(res);
    capAlloc.freeEmpty(port.cap());

    /* move tables */
    MLOG_DETAIL(mlog::app, "move tables ...");
    res = myCS.move(pl, pm3.cap(), max_cap_depth, cs.cap(), pCapAlloc(), max_cap_depth).wait();
    TEST(res);
    capAlloc.freeEmpty(pm3.cap());

    res = myCS.move(pl, pm2.cap(), max_cap_depth, cs.cap(), pCapAlloc(), max_cap_depth).wait();
    TEST(res);
    capAlloc.freeEmpty(pm2.cap());

    res = myCS.move(pl, pm10.cap(), max_cap_depth, cs.cap(), pCapAlloc(), max_cap_depth).wait();
    TEST(res);
    capAlloc.freeEmpty(pm10.cap());

    res = myCS.move(pl, pm11.cap(), max_cap_depth, cs.cap(), pCapAlloc(), max_cap_depth).wait();
    TEST(res);
    capAlloc.freeEmpty(pm11.cap());

    res = myCS.move(pl, pm12.cap(), max_cap_depth, cs.cap(), pCapAlloc(), max_cap_depth).wait();
    TEST(res);
    capAlloc.freeEmpty(pm12.cap());

    res = myCS.move(pl, pm13.cap(), max_cap_depth, cs.cap(), pCapAlloc(), max_cap_depth).wait();
    TEST(res);
    capAlloc.freeEmpty(pm13.cap());

    res = myCS.move(pl, pm14.cap(), max_cap_depth, cs.cap(), pCapAlloc(), max_cap_depth).wait();
    TEST(res);
    capAlloc.freeEmpty(pm14.cap());

    res = myCS.move(pl, pm15.cap(), max_cap_depth, cs.cap(), pCapAlloc(), max_cap_depth).wait();
    TEST(res);
    capAlloc.freeEmpty(pm15.cap());

    res = myCS.move(pl, pm16.cap(), max_cap_depth, cs.cap(), pCapAlloc(), max_cap_depth).wait();
    TEST(res);
    capAlloc.freeEmpty(pm16.cap());

    res = myCS.move(pl, pm17.cap(), max_cap_depth, cs.cap(), pCapAlloc(), max_cap_depth).wait();
    TEST(res);
    capAlloc.freeEmpty(pm17.cap());

    res = myCS.move(pl, pm4.cap(), max_cap_depth, cs.cap(), init::PML4, max_cap_depth).wait();
    TEST(res);
    capAlloc.freeEmpty(pm4.cap());

    /* wake EC */
    MLOG_DETAIL(mlog::app, "start process ...");
    MLOG_DETAIL(mlog::app, "   move EC");
    res = myCS.move(pl, ec.cap(), max_cap_depth, cs.cap(), init::EC, max_cap_depth).wait();
    TEST(res);

    MLOG_DETAIL(mlog::app, "   create reference");
    res = cs.reference(pl, init::EC, max_cap_depth, init::CSPACE, ec.cap(), max_cap_depth, 0).wait();
    TEST(res);

    MLOG_DETAIL(mlog::app, "   wake EC");
    res = ec.resume(pl).wait();
    TEST(res);

    MLOG_DETAIL(mlog::app, "   remove EC");
    capAlloc.free(ec.cap(), pl);

    return cs.cap();
  }

  private:
  CapMap cs;
  SimpleCapAlloc<init::CAP_ALLOC_START
    , init::CAP_ALLOC_END - init::CAP_ALLOC_START> pCapAlloc;
  elf64::Elf64Image img;
};


