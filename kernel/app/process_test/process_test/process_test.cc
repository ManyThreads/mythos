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
 * Copyright 2020 Philipp Gypser and contributors, BTU Cottbus-Senftenberg
 */

#include "mythos/init.hh"
#include "mythos/invocation.hh"
#include "mythos/InfoFrame.hh"
#include "runtime/Portal.hh"
#include "runtime/ExecutionContext.hh"
#include "runtime/CapMap.hh"
#include "runtime/RaplDriverIntel.hh"
#include "runtime/Example.hh"
#include "runtime/PageMap.hh"
#include "runtime/KernelMemory.hh"
#include "runtime/ThreadTeam.hh"
#include "runtime/SimpleCapAlloc.hh"
#include "runtime/tls.hh"
#include "runtime/mlog.hh"
#include "runtime/InterruptControl.hh"
#include <cstdint>
#include "util/optional.hh"
#include "util/align.hh"
#include "runtime/umem.hh"
#include "runtime/Mutex.hh"
#include "runtime/thread-extra.hh"
#include "mythos/caps.hh"


mythos::InfoFrame* info_ptr asm("info_ptr");
int main() asm("main");

constexpr uint64_t stacksize = 4*4096;
char initstack[stacksize];
char* initstack_top = initstack+stacksize;

mythos::Portal portal(mythos::init::PORTAL, info_ptr->getInvocationBuf());
mythos::Frame infoFrame(mythos::init::INFO_FRAME);
mythos::CapMap myCS(mythos::init::CSPACE);
mythos::PageMap myAS(mythos::init::PML4);
mythos::KernelMemory kmem(mythos::init::KM);
mythos::KObject device_memory(mythos::init::DEVICE_MEM);
mythos::SimpleCapAlloc< mythos::init::APP_CAP_START
  , mythos::init::SIZE-mythos::init::APP_CAP_START> capAlloc(myCS);
mythos::RaplDriverIntel rapl(mythos::init::RAPL_DRIVER_INTEL);
mythos::ThreadTeam team(mythos::init::THREAD_TEAM);


int main()
{
  MLOG_ERROR(mlog::app, "New process started :)", DVAR(sizeof(mythos::InfoFrame)), DVAR(mythos_get_pthread_ec_self()));

  MLOG_INFO(mlog::app, "info frame", DVARhex(info_ptr), DVAR(info_ptr->getNumThreads()), DVAR(info_ptr->getPsPerTSC()));

  mythos::PortalLock pl(portal);
  auto size = 32*1024*1024; // 2 MB
  auto align = 2*1024*1024; // 2 MB
  uintptr_t vaddr = mythos::round_up(info_ptr->getInfoEnd() + mythos::align2M,  mythos::align2M);
  // allocate a 2MiB frame
  mythos::Frame f(capAlloc());
  auto res2 = f.create(pl, kmem, size, align).wait();
  TEST(res2);
  // map the frame into our address space
  auto res3 = myAS.mmap(pl, f, vaddr, size, 0x1).wait();
  TEST(res3);
  MLOG_INFO(mlog::app, "mmap frame", DVAR(res3.state()),
            DVARhex(res3->vaddr), DVAR(res3->level));
  mythos::heap.addRange(vaddr, size);

  MLOG_ERROR(mlog::app, "process finished :(");
  return 0;
}
