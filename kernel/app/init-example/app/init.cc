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

#include "mythos/init.hh"
#include "mythos/invocation.hh"
#include "mythos/protocol/CpuDriverKNC.hh"
#include "mythos/PciMsgQueueMPSC.hh"
#include "runtime/Portal.hh"
#include "runtime/ExecutionContext.hh"
#include "runtime/CapMap.hh"
#include "runtime/Example.hh"
#include "runtime/PageMap.hh"
#include "runtime/KernelMemory.hh"
#include "runtime/SimpleCapAlloc.hh"
#include "runtime/tls.hh"
#include "runtime/mlog.hh"
#include <cstdint>
#include "util/optional.hh"
#include "runtime/umem.hh"

mythos::InvocationBuf* msg_ptr asm("msg_ptr");
int main() asm("main");

constexpr uint64_t stacksize = 4*4096;
char initstack[stacksize];
char* initstack_top = initstack+stacksize;

mythos::Portal portal(mythos::init::PORTAL, msg_ptr);
mythos::CapMap myCS(mythos::init::CSPACE);
mythos::PageMap myAS(mythos::init::PML4);
mythos::KernelMemory kmem(mythos::init::KM);
mythos::SimpleCapAllocDel capAlloc(portal, myCS, mythos::init::APP_CAP_START,
                                  mythos::init::SIZE-mythos::init::APP_CAP_START);

char threadstack[stacksize];
char* thread1stack_top = threadstack+stacksize/2;
char* thread2stack_top = threadstack+stacksize;

void* thread_main(void* ctx)
{
  MLOG_INFO(mlog::app, "hello thread!", DVAR(ctx));
  mythos::ISysretHandler::handle(mythos::syscall_wait());
  MLOG_INFO(mlog::app, "thread resumed from wait", DVAR(ctx));
  return 0;
}

void test_Example()
{
  char const obj[] = "hello object!";
  MLOG_ERROR(mlog::app, "test_Example begin");
  mythos::PortalLock pl(portal); // future access will fail if the portal is in use already
  mythos::Example example(capAlloc());
  TEST(example.create(pl, kmem).wait()); // use default mythos::init::EXAMPLE_FACTORY
  // wait() waits until the result is ready and returns a copy of the data and state.
  // hence, the contents of res1 are valid even after the next use of the portal
  TEST(example.printMessage(pl, obj, sizeof(obj)-1).wait());
  TEST(capAlloc.free(example,pl));
  // pl.release(); // implicit by PortalLock's destructor
  MLOG_ERROR(mlog::app, "test_Example end");
}

void test_Portal()
{
  MLOG_ERROR(mlog::app, "test_Portal begin");
  mythos::PortalLock pl(portal); // future access will fail if the portal is in use already
  MLOG_INFO(mlog::app, "test_Portal: allocate portal");
  uintptr_t vaddr = 22*1024*1024; // choose address different from invokation buffer
  // allocate a portal
  mythos::Portal p2(capAlloc(), (void*)vaddr);
  auto res1 = p2.create(pl, kmem).wait();
  TEST(res1);
  // allocate a 2MiB frame
  MLOG_INFO(mlog::app, "test_Portal: allocate frame");
  mythos::Frame f(capAlloc());
  auto res2 = f.create(pl, kmem, 2*1024*1024, 2*1024*1024).wait();
  MLOG_INFO(mlog::app, "alloc frame", DVAR(res2.state()));
  TEST(res2);
  // map the frame into our address space
  MLOG_INFO(mlog::app, "test_Portal: map frame");
  auto res3 = myAS.mmap(pl, f, vaddr, 2*1024*1024, 0x1).wait();
  MLOG_INFO(mlog::app, "mmap frame", DVAR(res3.state()),
            DVARhex(res3->vaddr), DVARhex(res3->size), DVAR(res3->level));
  TEST(res3);
  // bind the portal in order to receive responses
  MLOG_INFO(mlog::app, "test_Portal: configure portal");
  auto res4 = p2.bind(pl, f, 0, mythos::init::EC).wait();
  TEST(res4);
  // and delete everything again
  MLOG_INFO(mlog::app, "test_Portal: delete frame");
  TEST(capAlloc.free(f, pl));
  MLOG_INFO(mlog::app, "test_Portal: delete portal");
  TEST(capAlloc.free(p2, pl));
  MLOG_ERROR(mlog::app, "test_Portal end");
}

void test_float()
{
  MLOG_INFO(mlog::app, "testing user-mode floating point");

  volatile float x = 5.5;
  volatile float y = 0.5;

  float z = x*y;

  TEST_EQ(int(z), 2);
  TEST_EQ(int(1000*(z-float(int(z)))), 750);
  MLOG_INFO(mlog::app, "float z:", int(z), ".", int(1000*(z-float(int(z)))));
}


uint64_t readFS(uintptr_t offset) {
  uint64_t value;
  asm ("movq %%fs:%1, %0" : "=r" (value) : "m" (*(char*)offset));
  return value;
}

thread_local int x = 1024;
thread_local int y = 2048;
void test_tls()
{
  MLOG_INFO(mlog::app, "testing thread local storage");
  MLOG_INFO(mlog::app, "main thread TLS:", DVARhex(readFS(0)), DVARhex(readFS(0x28)));
  
  mythos::PortalLock pl(portal);
  TEST_EQ(x, 1024); // just testing if access through %fs is successful
  TEST_EQ(y, 2048);
  x = 2*x;
  y = 2*y;
  TEST_EQ(x, 2048);
  TEST_EQ(y, 4096);

  auto threadFun = [] (void *data) -> void* {
    MLOG_INFO(mlog::app, "main thread TLS:", DVARhex(readFS(0)), DVARhex(readFS(0x28)));
    TEST_EQ(x, 1024);
    TEST_EQ(y, 2048);
    mythos::syscall_wait();
    TEST_EQ(x, 1024);
    TEST_EQ(y, 2048);
    x = x*2;
    y = y*2;
    TEST_EQ(x, 2048);
    TEST_EQ(y, 4096);
  };

  mythos::ExecutionContext ec1(capAlloc());
  auto tls = mythos::setupNewTLS();
  MLOG_INFO(mlog::app, "test_EC: create ec1 TLS", DVARhex(tls));
  ASSERT(tls != nullptr);
  auto res1 = ec1.create(kmem).as(myAS).cs(myCS).sched(mythos::init::SCHEDULERS_START + 1)
    .prepareStack(thread1stack_top).startFun(threadFun, nullptr)
    .suspended(false).fs(tls)
    .invokeVia(pl).wait();
  TEST(res1);
  TEST(ec1.setFSGS(pl,(uint64_t) tls, 0).wait());
  mythos::syscall_notify(ec1.cap());
}

void test_heap() {
  MLOG_INFO(mlog::app, "Test heap");
  mythos::PortalLock pl(portal);
  uintptr_t vaddr = 22*1024*1024; // choose address different from invokation buffer
  auto size = 4*1024*1024; // 2 MB
  auto align = 2*1024*1024; // 2 MB
  // allocate a 2MiB frame
  mythos::Frame f(capAlloc());
  auto res2 = f.create(pl, kmem, size, align).wait();
  TEST(res2);
  // map the frame into our address space
  auto res3 = myAS.mmap(pl, f, vaddr, size, 0x1).wait();
  TEST(res3);
  MLOG_INFO(mlog::app, "mmap frame", DVAR(res3.state()),
            DVARhex(res3->vaddr), DVARhex(res3->size), DVAR(res3->level));
  mythos::heap.addRange(vaddr, size);
  for (int i : {10, 100, 1000, 10000, 100000}) {
    int *a = new int[i];
    delete a;
  }
  
//   int* tests[10];
//   for (int i=0;i<10;i++) {
//       tests[i] = new int[100];
//       TEST(tests[i] != nullptr);
//   }
//   for (int i=0;i<10;i++) {
//       delete tests[i];
//   }
  
  MLOG_INFO(mlog::app, "End Test heap");
}

struct HostChannel {
  void init() { ctrlToHost.init(); ctrlFromHost.init(); }
  typedef mythos::PCIeRingChannel<128,8> CtrlChannel;
  CtrlChannel ctrlToHost;
  CtrlChannel ctrlFromHost;
};

mythos::PCIeRingProducer<HostChannel::CtrlChannel> app2host;
mythos::PCIeRingConsumer<HostChannel::CtrlChannel> host2app;

int main()
{
  char const str[] = "hello world!";
  char const end[] = "bye, cruel world!";
  mythos::syscall_debug(str, sizeof(str)-1);
  MLOG_ERROR(mlog::app, "application is starting :)", DVARhex(msg_ptr), DVARhex(initstack_top));

  test_float();
  test_Example();
  test_Portal();
  test_heap(); // heap must be initialized for tls test
  test_tls();

  {
    mythos::PortalLock pl(portal); // future access will fail if the portal is in use already
    // allocate a 2MiB frame
    mythos::Frame hostChannelFrame(capAlloc());
    auto res1 = hostChannelFrame.create(pl, kmem, 2*1024*1024, 2*1024*1024).wait();
    MLOG_INFO(mlog::app, "alloc hostChannel frame", DVAR(res1.state()));
    TEST(res1);

    // map the frame into our address space
    uintptr_t vaddr = 24*1024*1024;
    auto res2 = myAS.mmap(pl, hostChannelFrame, vaddr, 2*1024*1024, 0x1).wait();
    MLOG_INFO(mlog::app, "mmap hostChannel frame", DVAR(res2.state()),
              DVARhex(res2.get().vaddr), DVARhex(res2.get().size), DVAR(res2.get().level));
    TEST(res2);

    // initialise the memory
    HostChannel* hostChannel = reinterpret_cast<HostChannel*>(vaddr);
    hostChannel->init();
    host2app.setChannel(&hostChannel->ctrlFromHost);
    app2host.setChannel(&hostChannel->ctrlToHost);

    // register the frame in the host info table
    // auto res3 = mythos::PortalFuture<void>(pl.invoke<mythos::protocol::CpuDriverKNC::SetInitMem>(mythos::init::CPUDRIVER, hostChannelFrame.cap())).wait();
    // MLOG_INFO(mlog::app, "register at host info table", DVAR(res3.state()));
    //ASSERT(res3.state() == mythos::Error::SUCCESS);
  }

  mythos::ExecutionContext ec1(capAlloc());
  mythos::ExecutionContext ec2(capAlloc());
  {
    MLOG_INFO(mlog::app, "test_EC: create ec1");
    mythos::PortalLock pl(portal); // future access will fail if the portal is in use already

    auto tls1 = mythos::setupNewTLS();
    ASSERT(tls1 != nullptr);
    auto res1 = ec1.create(kmem).as(myAS).cs(myCS).sched(mythos::init::SCHEDULERS_START)
    .prepareStack(thread1stack_top).startFun(&thread_main, nullptr)
    .suspended(false).fs(tls1)
    .invokeVia(pl).wait();
    TEST(res1);

    MLOG_INFO(mlog::app, "test_EC: create ec2");
    auto tls2 = mythos::setupNewTLS();
    ASSERT(tls2 != nullptr);
    auto res2 = ec2.create(kmem).as(myAS).cs(myCS).sched(mythos::init::SCHEDULERS_START+1)
    .prepareStack(thread2stack_top).startFun(&thread_main, nullptr)
    .suspended(false).fs(tls2)
    .invokeVia(pl).wait();
    TEST(res2);

}

  for (volatile int i=0; i<100000; i++) {
    for (volatile int j=0; j<1000; j++) {}
  }

  MLOG_INFO(mlog::app, "sending notifications");
  mythos::syscall_notify(ec1.cap());
  mythos::syscall_notify(ec2.cap());

  mythos::syscall_debug(end, sizeof(end)-1);

  return 0;
}
