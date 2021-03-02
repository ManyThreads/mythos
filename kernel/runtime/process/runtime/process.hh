#pragma once

#include "mythos/init.hh"
#include "util/assert.hh"
#include "mythos/caps.hh"
#include "runtime/mlog.hh"
#include "util/optional.hh"
#include "util/elf64.hh"
#include "util/align.hh"
#include "util/events.hh"
#include "mythos/InfoFrame.hh"
#include "runtime/CapAlloc.hh"
#include "runtime/ThreadTeam.hh"
#include "runtime/thread-extra.hh"
#include "mythos/syscall.hh"
#include <vector>

extern mythos::InfoFrame* info_ptr asm("info_ptr");
extern mythos::CapMap myCS;
extern mythos::PageMap myAS;
extern mythos::KernelMemory kmem;
extern mythos::ThreadTeam team;

class ProcessExitEvent;
extern mythos::Event<> groupExit;
extern ProcessExitEvent processExitEvent;

using namespace mythos;

class ProcessExitEvent : public EventHook<> {
  public:
  ProcessExitEvent(){
    groupExit.add(this);
  }

  void processEvent() override {
    MLOG_ERROR(mlog::app, __PRETTY_FUNCTION__);
    info_ptr->setRunning(false);
    auto parent = info_ptr->getParent();
    if(parent != null_cap){
        mythos::syscall_signal(parent);
    }
  }
};

class Process{
  public:
  Process(char* image)
    : cs(capAlloc())
    , pCapAlloc(cs)
    , img(image)
    , pInfoFrame(nullptr)
    , parent(mythos_get_pthread_ec_self())
  {}
  
  void wait(){
    MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
    if(pInfoFrame){
      if(pInfoFrame->isRunning()){
        ASSERT(parent == mythos_get_pthread_ec_self());
        //pInfoFrame->setParent(parent);
        pInfoFrame->setParent(init::PARENT_EC);
        while(pInfoFrame->isRunning()){
          mythos_wait();
        }
        pInfoFrame->setParent(null_cap);
      }
    }
    MLOG_DETAIL(mlog::app, "wait return");
  }

  void remove(PortalLock& pl){
    MLOG_ERROR(mlog::app, __PRETTY_FUNCTION__);
    pInfoFrame = nullptr;
    MLOG_ERROR(mlog::app, "free capmap");
    capAlloc.free(cs, pl);
    MLOG_ERROR(mlog::app, "free dynamically allocated caps in own CS");
    for (std::vector<CapPtr>::reverse_iterator i = caps.rbegin(); 
        i != caps.rend(); ++i ) {
      MLOG_ERROR(mlog::app, DVAR(*i));
        capAlloc.free(*i, pl);
     } 
    caps.clear();
  }

  void join(PortalLock& pl){
    wait();
    remove(pl);
  }

  ~Process(){}

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
      ASSERT(res);

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
      caps.push_back(f.cap());
      
      uintptr_t tmp_vaddr = 42*align512G;
      MLOG_DETAIL(mlog::app, "temporary map frame to own address space ...", DVARhex(tmp_vaddr));
      MLOG_DETAIL(mlog::app, "   create PageMap");
      PageMap pm3(capAlloc());
      res = pm3.create(pl, kmem, 3).wait();
      ASSERT(res);
      PageMap pm2(capAlloc());
      res = pm2.create(pl, kmem, 2).wait();
      ASSERT(res);

      MLOG_DETAIL(mlog::app, "   installMap");
      res = myAS.installMap(pl, pm3, ((tmp_vaddr >> 39) & 0x1FF)<< 39, 4,
        protocol::PageMap::MapFlags().writable(true).configurable(true)).wait();
      ASSERT(res);
      res = pm3.installMap(pl, pm2, ((tmp_vaddr >> 30) & 0x1FF) << 30, 3,
        protocol::PageMap::MapFlags().writable(true).configurable(true)).wait();
      ASSERT(res);

      MLOG_DETAIL(mlog::app, "   mmap");
      res = myAS.mmap(pl, f, tmp_vaddr, size, 0x1).wait();
      ASSERT(res);

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
      ASSERT(res);

      MLOG_DETAIL(mlog::app, "   remove maps");
      res = myAS.removeMap(pl, tmp_vaddr, 2).wait();
      ASSERT(res);
      res = myAS.removeMap(pl, tmp_vaddr, 3).wait();
      ASSERT(res);

      MLOG_DETAIL(mlog::app, "   move frame");
      res = myCS.reference(pl, f.cap(), max_cap_depth, cs.cap(), pCapAlloc(), max_cap_depth).wait();
      ASSERT(res);

      MLOG_DETAIL(mlog::app, "   delete tables");
      capAlloc.free(pm3.cap(), pl);
      capAlloc.free(pm2.cap(), pl);

      return ipc_addr;
  }

  optional<CapPtr> createProcess(PortalLock& pl){
    MLOG_INFO(mlog::app, __func__);
    MLOG_ERROR(mlog::app, "process needs to be reworked!");
    return 0;
    /* create CapMap */
    MLOG_DETAIL(mlog::app, "create CapMap ...");
    auto res = cs.create(pl, kmem, CapPtrDepth(12), CapPtrDepth(20), CapPtr(0)).wait();
    ASSERT(res);
    MLOG_DETAIL(mlog::app, "set CapMap reference...");
    res = myCS.reference(pl, cs.cap(), max_cap_depth, cs.cap(), init::CSPACE, max_cap_depth, 1).wait();
    ASSERT(res);
    
     //copy relevant caps 
    MLOG_DETAIL(mlog::app, "copy relevant Caps ...");
    MLOG_DETAIL(mlog::app, "   kernel memory");
    res = myCS.reference(pl, init::KM, max_cap_depth, cs.cap(), init::KM, max_cap_depth, 1).wait();
    ASSERT(res);

    MLOG_DETAIL(mlog::app, "   factories");
    for(CapPtr ptr = init::EXAMPLE_FACTORY; ptr <= init::UNTYPED_MEMORY_FACTORY; ptr++){
      res = myCS.reference(pl, ptr, max_cap_depth, cs.cap(), ptr, max_cap_depth).wait();
      ASSERT(res);
    }

    MLOG_DETAIL(mlog::app, "   DEVICE_MEMORY");
    res = myCS.reference(pl, init::DEVICE_MEM, max_cap_depth, cs.cap(), init::DEVICE_MEM, max_cap_depth, 0).wait();
    ASSERT(res);

    MLOG_DETAIL(mlog::app, "   RAPL driver");
    res = myCS.reference(pl, init::RAPL_DRIVER_INTEL, max_cap_depth, cs.cap(), init::RAPL_DRIVER_INTEL, max_cap_depth, 0).wait();
    ASSERT(res);

    MLOG_DETAIL(mlog::app, "   Interrupt control");
    MLOG_WARN(mlog::app, "SKIP: Interrupt control caps!");
    //todo: how to check whether cap is existing?
    //for(CapPtr ptr = init::INTERRUPT_CONTROL_START; ptr < init::INTERRUPT_CONTROL_END; ptr++){
      //res = myCS.reference(pl, ptr, max_cap_depth, cs.cap(), ptr, max_cap_depth, 0).wait();
      //ASSERT(res);
    //}

    /* create address space */
    //todo: use dynamic table allocation in mmap!
    MLOG_DETAIL(mlog::app, "create PageMaps ...");
    uintptr_t elf_vaddr = 0x400000;

    // create tables
    PageMap pm4(capAlloc());
    caps.push_back(pm4.cap());
    auto res_pm = pm4.create(pl, kmem, 4).wait();
    ASSERT(res_pm);
    PageMap pm3(capAlloc());
    caps.push_back(pm3.cap());
    res_pm = pm3.create(pl, kmem, 3).wait();
    ASSERT(res_pm);
    PageMap pm2(capAlloc());
    caps.push_back(pm2.cap());
    res_pm = pm2.create(pl, kmem, 2).wait();
    ASSERT(res_pm);
    PageMap pm10(capAlloc());
    caps.push_back(pm10.cap());
    res_pm = pm10.create(pl, kmem, 1).wait();
    ASSERT(res_pm);
    PageMap pm11(capAlloc());
    caps.push_back(pm11.cap());
    res_pm = pm11.create(pl, kmem, 1).wait();
    ASSERT(res_pm);
    PageMap pm12(capAlloc());
    caps.push_back(pm12.cap());
    res_pm = pm12.create(pl, kmem, 1).wait();
    ASSERT(res_pm);
    PageMap pm13(capAlloc());
    caps.push_back(pm13.cap());
    res_pm = pm13.create(pl, kmem, 1).wait();
    ASSERT(res_pm);
    PageMap pm14(capAlloc());
    caps.push_back(pm14.cap());
    res_pm = pm14.create(pl, kmem, 1).wait();
    ASSERT(res_pm);
    PageMap pm15(capAlloc());
    caps.push_back(pm15.cap());
    res_pm = pm15.create(pl, kmem, 1).wait();
    ASSERT(res_pm);
    PageMap pm16(capAlloc());
    caps.push_back(pm16.cap());
    res_pm = pm16.create(pl, kmem, 1).wait();
    ASSERT(res_pm);
    PageMap pm17(capAlloc());
    caps.push_back(pm17.cap());
    res_pm = pm17.create(pl, kmem, 1).wait();
    ASSERT(res_pm);
    PageMap pm18(capAlloc());
    caps.push_back(pm18.cap());
    res_pm = pm18.create(pl, kmem, 1).wait();
    ASSERT(res_pm);
    PageMap pm19(capAlloc());
    caps.push_back(pm19.cap());
    res_pm = pm19.create(pl, kmem, 1).wait();
    ASSERT(res_pm);

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
    pm2.installMap(pl, pm18, ((elf_vaddr >> 21) & 0x1FF) + 8 << 21, 2,
      mythos::protocol::PageMap::MapFlags().writable(true).configurable(true)).wait();
    pm2.installMap(pl, pm19, ((elf_vaddr >> 21) & 0x1FF) + 9 << 21, 2,
      mythos::protocol::PageMap::MapFlags().writable(true).configurable(true)).wait();

    /* load image */
    auto ipc_vaddr = loadImage(pl, pm4);
    ASSERT(ipc_vaddr);

    /* create InfoFrame */
    MLOG_DETAIL(mlog::app, "create InfoFrame ...");
    auto size = round_up(sizeof(InfoFrame), align2M);
    MLOG_DETAIL(mlog::app, "   create frame");
    Frame iFrame(capAlloc());
    caps.push_back(iFrame.cap());
    res = iFrame.create(pl, kmem, size, align2M).wait();
    ASSERT(res);
    res = myCS.reference(pl, iFrame.cap(), max_cap_depth, cs.cap(), init::INFO_FRAME, max_cap_depth, 0).wait();
    ASSERT(res);
    MLOG_WARN(mlog::app, "   map frame to target page map", DVARhex(size), DVARhex(*ipc_vaddr));
    res = pm4.mmap(pl, iFrame, *ipc_vaddr, size, 0x1).wait();
    ASSERT(res);
    
    uintptr_t tmp_vaddr_if = 11*align512G;
    MLOG_DETAIL(mlog::app, "   temporally map frame to own page map", DVARhex(tmp_vaddr_if));
    PageMap pm3if(capAlloc());
    caps.push_back(pm3if.cap());
    res = pm3if.create(pl, kmem, 3).wait();
    ASSERT(res);
    PageMap pm2if(capAlloc());
    caps.push_back(pm2if.cap());
    res = pm2if.create(pl, kmem, 2).wait();
    ASSERT(res);
    MLOG_DETAIL(mlog::app, "   installMap");
    res = myAS.installMap(pl, pm3if, ((tmp_vaddr_if >> 39) & 0x1FF)<< 39, 4,
      protocol::PageMap::MapFlags().writable(true).configurable(true)).wait();
    ASSERT(res);
    res = pm3if.installMap(pl, pm2if, ((tmp_vaddr_if >> 30) & 0x1FF) << 30, 3,
      protocol::PageMap::MapFlags().writable(true).configurable(true)).wait();
    ASSERT(res);

    MLOG_DETAIL(mlog::app, "   mmap");
    res = myAS.mmap(pl, iFrame, tmp_vaddr_if, size, 0x1).wait();
    ASSERT(res);
    
    MLOG_DETAIL(mlog::app, "   copy info frame content");
    mythos::memcpy(reinterpret_cast<void*>(tmp_vaddr_if), info_ptr, sizeof(InfoFrame));

    pInfoFrame = reinterpret_cast<InfoFrame*>(tmp_vaddr_if);
    pInfoFrame->setRunning(true);
    pInfoFrame->setParent(null_cap);

    res = myCS.reference(pl, parent, max_cap_depth, cs.cap(), init::PARENT_EC, max_cap_depth).wait();
    ASSERT(res);

    /* create EC */
    MLOG_DETAIL(mlog::app, "create EC ...", DVARhex(img.header()->entry));
    ExecutionContext ec(capAlloc());
    res = ec.create(kmem).as(pm4).cs(cs)
    .rawFun(reinterpret_cast<int (*)(void*)>(img.header()->entry), reinterpret_cast<void*>(*ipc_vaddr))
    .suspended(true)
    .invokeVia(pl).wait();
    caps.push_back(ec.cap());
    ASSERT(res);

    /* create ThreadTeam */
    MLOG_DETAIL(mlog::app, "create ThreadTeam ...");
    ThreadTeam tt(capAlloc());
    res = tt.create(pl, kmem, init::PROCESSOR_ALLOCATOR).wait();
    caps.push_back(tt.cap());
    ASSERT(res);
    MLOG_DETAIL(mlog::app, "   register EC in ThreadTeam");
    res = tt.tryRunEC(pl, ec).wait();
    ASSERT(res);
    res = myCS.reference(pl, tt.cap(), max_cap_depth, cs.cap(), init::THREAD_TEAM, max_cap_depth).wait();
    ASSERT(res);

    /* create portal */
    MLOG_DETAIL(mlog::app, "create Portal ...");
    MLOG_DETAIL(mlog::app, "   map frame at ", DVARhex(*ipc_vaddr));

    MLOG_DETAIL(mlog::app, "   create");
    Portal port(capAlloc(), reinterpret_cast<void*>(*ipc_vaddr));
    res = port.create(pl, kmem).wait();
    caps.push_back(port.cap());
    ASSERT(res);
    res = port.bind(pl, iFrame, 0, ec.cap()).wait();
    ASSERT(res);

    MLOG_DETAIL(mlog::app, "   move Portal");
    res = myCS.reference(pl, port.cap(), max_cap_depth, cs.cap(), init::PORTAL, max_cap_depth).wait();
    ASSERT(res);
    
    /* move tables */
    MLOG_DETAIL(mlog::app, "reference pm4 ...");
    res = myCS.reference(pl, pm4.cap(), max_cap_depth, cs.cap(), init::PML4, max_cap_depth).wait();

    /* wake EC */
    MLOG_DETAIL(mlog::app, "start process ...");
    MLOG_DETAIL(mlog::app, "   move EC");
    res = myCS.reference(pl, ec.cap(), max_cap_depth, cs.cap(), init::EC, max_cap_depth).wait();
    ASSERT(res);

    MLOG_DETAIL(mlog::app, "   wake EC");
    res = ec.resume(pl).wait();
    ASSERT(res);

    return cs.cap();
  }

  private:
  CapMap cs;
  SimpleCapAlloc<init::CAP_ALLOC_START
    , init::CAP_ALLOC_END - init::CAP_ALLOC_START> pCapAlloc;
  elf64::Elf64Image img;
  InfoFrame* pInfoFrame;
  CapPtr parent;
  std::vector<CapPtr> caps;
};


