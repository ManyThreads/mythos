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
  char const str[] = "hello thread!";
  mythos::syscall_debug(str, sizeof(str)-1);
  return 0;
}

int main()
{
  char const str[] = "hello world!";
  char const obj[] = "hello object!";
  char const end[] = "bye, cruel world!";
  mythos::syscall_debug(str, sizeof(str)-1);

  {
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
  }

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

  mythos::syscall_debug(end, sizeof(end)-1);

  return 0;
}
