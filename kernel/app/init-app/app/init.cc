/* -*- mode:C++; indent-tabs-mode:nil; -*- */
/* MyThOS: The Many-Threads Operating System
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
#include "runtime/Portal.hh"
#include "runtime/ExecutionContext.hh"
#include "runtime/CapMap.hh"
#include "runtime/Example.hh"
#include "runtime/PageMap.hh"
#include "runtime/UntypedMemory.hh"
#include "app/mlog.hh"
#include <cstdint>
#include "util/optional.hh"

mythos::InvocationBuf* msg_ptr asm("msg_ptr");
int main() asm("main");

constexpr uint64_t stacksize = 4*4096;
char initstack[stacksize];
char* initstack_top = initstack+stacksize;

mythos::Portal portal(mythos::init::PORTAL, msg_ptr);
mythos::CapMap myCS(mythos::init::CSPACE);
mythos::PageMap myAS(mythos::init::PML4);
mythos::UntypedMemory kmem(mythos::init::UM);

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
  MLOG_ERROR(mlog::app, "test_Example begin");
  char const obj[] = "hello object!";
  mythos::Example example(mythos::init::APP_CAP_START);
  auto res1 = example.create(portal, kmem, mythos::init::EXAMPLE_FACTORY);
  res1.wait();
  ASSERT(res1.state() == mythos::Error::SUCCESS);
  auto res2 = example.printMessage(res1.reuse(), obj, sizeof(obj)-1);
  res2.wait();
  ASSERT(res2.state() == mythos::Error::SUCCESS);
  auto res3 = myCS.deleteCap(res2.reuse(), example);
  res3.wait();
  ASSERT(res3.state() == mythos::Error::SUCCESS);
  MLOG_ERROR(mlog::app, "test_Example end");
}

void test_Portal()
{
  MLOG_ERROR(mlog::app, "test_Portal begin");
  MLOG_INFO(mlog::app, "test_Portal: allocate portal");
  uintptr_t vaddr = 20*1024*1024;
  // allocate a portal
  mythos::Portal p2(mythos::init::APP_CAP_START, (void*)vaddr);
  auto res1 = p2.create(portal, kmem);
  res1.wait();
  ASSERT(res1);

  // allocate a 2MiB frame
  MLOG_INFO(mlog::app, "test_Portal: allocate frame");
  mythos::Frame f(mythos::init::APP_CAP_START+1);
  auto res2 = f.create(res1.reuse(), kmem, mythos::init::MEMORY_REGION_FACTORY,
                       2*1024*1024, 2*1024*1024);
  res2.wait();
  MLOG_INFO(mlog::app, "alloc frame", DVAR(res2.state()));
  ASSERT(res2);

  // map the frame into our address space
  MLOG_INFO(mlog::app, "test_Portal: map frame");
  auto res3 = myAS.mmap(res2.reuse(), f, vaddr, 2*1024*1024, 0x1);
  MLOG_INFO(mlog::app, "mmap frame", DVAR(res3.state()),
            DVARhex(res3->vaddr), DVARhex(res3->size), DVAR(res3->level));
  res3.wait();
  ASSERT(res3);

  // bind the portal in order to receive responses
  MLOG_INFO(mlog::app, "test_Portal: configure portal");
  auto res4 = p2.bind(res3.reuse(), f, 0, mythos::init::EC);
  res4.wait();
  ASSERT(res4);

  // and delete everything again
  MLOG_INFO(mlog::app, "test_Portal: delete frame");
  auto res5 = myCS.deleteCap(res4.reuse(), f);
  res5.wait();
  ASSERT(res5);
  MLOG_INFO(mlog::app, "test_Portal: delete portal");
  auto res6 = myCS.deleteCap(res5.reuse(), p2);
  res6.wait();
  ASSERT(res6);
  MLOG_ERROR(mlog::app, "test_Portal end");
}


int main()
{
  char const str[] = "hello world!";
  char const end[] = "bye, cruel world!";
  mythos::syscall_debug(str, sizeof(str)-1);

  //test_Example();
  //test_Portal();

  mythos::ExecutionContext ec1(mythos::init::APP_CAP_START);
  auto res1 = ec1.create(portal, kmem, mythos::init::EXECUTION_CONTEXT_FACTORY,
                         myAS, myCS, mythos::init::SCHEDULERS_START,
                         thread1stack_top, &thread_main, nullptr);
  res1.wait();
  ASSERT(res1.state() == mythos::Error::SUCCESS);

  mythos::ExecutionContext ec2(mythos::init::APP_CAP_START+1);
  auto res2 = ec2.create(res1.reuse(), kmem, mythos::init::EXECUTION_CONTEXT_FACTORY,
                         myAS, myCS, mythos::init::SCHEDULERS_START+1,
                         thread2stack_top, &thread_main, nullptr);
  res2.wait();
  ASSERT(res2.state() == mythos::Error::SUCCESS);

  for (volatile int i=0; i<100000; i++) {
    for (volatile int j=0; j<1000; j++) {}
  }

  MLOG_INFO(mlog::app, "sending notifications");
  mythos::syscall_notify(ec1.cap());
  mythos::syscall_notify(ec2.cap());

  mythos::syscall_debug(end, sizeof(end)-1);

  return 0;
}
