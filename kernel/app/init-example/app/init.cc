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
#include "runtime/InterruptControl.hh"
#include <cstdint>
#include "util/optional.hh"
#include "runtime/umem.hh"
#include "runtime/Mutex.hh"
#include "mythos/protocol/PerfMon.hh"

#include <vector>
#include <array>
#include <cmath>

#include <pthread.h>
#include <omp.h>

mythos::InvocationBuf* msg_ptr asm("msg_ptr");
int main() asm("main");

constexpr uint64_t stacksize = 4*4096;
char initstack[stacksize];
char* initstack_top = initstack+stacksize;

mythos::Portal portal(mythos::init::PORTAL, msg_ptr);
mythos::CapMap myCS(mythos::init::CSPACE);
mythos::PageMap myAS(mythos::init::PML4);
mythos::KernelMemory kmem(mythos::init::KM);
mythos::KObject device_memory(mythos::init::DEVICE_MEM);
mythos::SimpleCapAllocDel capAlloc(portal, myCS, mythos::init::APP_CAP_START,
                                  mythos::init::SIZE-mythos::init::APP_CAP_START);

char threadstack[stacksize];
char* thread1stack_top = threadstack+stacksize/2;
char* thread2stack_top = threadstack+stacksize;

char threadstack2[stacksize];
char* thread3stack_top = threadstack2+stacksize/2;
char* thread4stack_top = threadstack2+stacksize;


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
  TEST(res2);
  MLOG_INFO(mlog::app, "alloc frame", DVAR(res2.state()));
  // map the frame into our address space
  MLOG_INFO(mlog::app, "test_Portal: map frame");
  auto res3 = myAS.mmap(pl, f, vaddr, 2*1024*1024, 0x1).wait();
  TEST(res3);
  MLOG_INFO(mlog::app, "mmap frame", DVAR(res3.state()),
            DVARhex(res3->vaddr), DVAR(res3->level));
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

void test_memory_root()
{
  // see example mapping in PluginCpuDriverKNC.cc
  auto ivt = reinterpret_cast<uint64_t*>(128ull*1024*1024*1024);
  MLOG_INFO(mlog::app, "test_memory_root: try read from init mapping", DVAR(ivt));
  auto val = *ivt;
  MLOG_INFO(mlog::app, "...got", DVARhex(val));
    
  uintptr_t paddr = 0xB8000; // text mode CGA screen
  uintptr_t vaddr = 22*1024*1024; // choose address different from invokation buffer

  MLOG_ERROR(mlog::app, "test_memory_root begin");
  mythos::PortalLock pl(portal); // future access will fail if the portal is in use already

  MLOG_INFO(mlog::app, "test_memory_root: allocate device memory");
  mythos::Frame f(capAlloc());
  auto res1 = f.createDevice(pl, device_memory, paddr, 2*1024*1024, true).wait();
  TEST(res1);
  MLOG_INFO(mlog::app, "alloc frame", DVAR(res1.state()));

  MLOG_INFO(mlog::app, "test_memory_root: allocate level 1 page map (4KiB pages)");
  mythos::PageMap p1(capAlloc());
  auto res2 = p1.create(pl, kmem, 1);
  TEST(res2);

  MLOG_INFO(mlog::app, "test_memory_root: map level 1 page map on level 2", DVARhex(vaddr));
  auto res3 = myAS.installMap(pl, p1, vaddr, 2,
                              mythos::protocol::PageMap::MapFlags().writable(true).configurable(true)).wait();
  TEST(res3);

  MLOG_INFO(mlog::app, "test_memory_root: map frame");
  auto res4 = myAS.mmap(pl, f, vaddr, 4096, mythos::protocol::PageMap::MapFlags().writable(true)).wait();
  MLOG_INFO(mlog::app, "mmap frame", DVAR(res4.state()),
            DVARhex(res4->vaddr), DVAR(res4->level));
  TEST(res4);

  struct CGAChar {
      CGAChar(uint8_t ch=0, uint8_t fg=15, uint8_t bg=0, uint8_t blink=0) : ch(ch&0xFF), fg(fg&0x0F),bg(bg&0x07),blink(blink&0x01) {} 
      uint16_t ch:8, fg:4, bg:3, blink:1;
    };
  auto screen = reinterpret_cast<CGAChar*>(vaddr);

  auto msg = "Device Memory is working :)";
  for (size_t i=0; i<strlen(msg); i++) screen[i] = CGAChar(msg[i], 12, 3, 1);

  MLOG_INFO(mlog::app, "test_memory_root: delete page map");
  TEST(capAlloc.free(p1, pl));
  MLOG_INFO(mlog::app, "test_memory_root: delete device frame");
  TEST(capAlloc.free(f, pl));
  MLOG_ERROR(mlog::app, "test_memory_root end");
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
    return nullptr;
  };

  mythos::ExecutionContext ec1(capAlloc());
  auto tls = mythos::setupNewTLS();
  MLOG_INFO(mlog::app, "test_EC: create ec1 TLS", DVARhex(tls));
  ASSERT(tls != nullptr);
  auto res1 = ec1.create(kmem).as(myAS).cs(myCS).sched(mythos::init::SCHEDULERS_START + 1)
    .prepareStack(thread1stack_top).startFun(threadFun, nullptr, ec1.cap())
    .suspended(false).fs(tls)
    .invokeVia(pl).wait();
  TEST(res1);
  TEST(ec1.setFSGS(pl,(uint64_t) tls, 0).wait());
  mythos::syscall_signal(ec1.cap());
  MLOG_INFO(mlog::app, "End test tls");
}


void test_heap() {
  MLOG_INFO(mlog::app, "Test heap");
  mythos::PortalLock pl(portal);
  uintptr_t vaddr = 22*1024*1024; // choose address different from invokation buffer
  auto size = 128*1024*1024;
  auto align = 2*1024*1024; // 2 MB
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

  // test allocation in standard template library
  std::vector<int> foo;
  for (int i=0; i<100; i++) foo.push_back(i);

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

bool test_HostChannel(mythos::Portal& portal, uintptr_t vaddr, size_t size)
{
  mythos::PortalLock pl(portal); // future access will fail if the portal is in use already
  // allocate a 2MiB frame
  mythos::Frame hostChannelFrame(capAlloc());
  auto res1 = hostChannelFrame.create(pl, kmem, size, size).wait();
  MLOG_INFO(mlog::app, "alloc hostChannel frame", DVAR(res1.state()));
  TEST(res1);

  // map the frame into our address space
  auto res2 = myAS.mmap(pl, hostChannelFrame, vaddr, size, 0x1).wait();
  MLOG_INFO(mlog::app, "mmap hostChannel frame", DVAR(res2.state()),
            DVARhex(res2.get().vaddr), DVAR(res2.get().level));
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

  return true;
}

void test_exceptions() {
  MLOG_INFO(mlog::app, "Test Exceptions");
    try{
        MLOG_INFO(mlog::app, "throwing 42");
        throw 42;
    } catch (int i){
        MLOG_INFO(mlog::app, "catched", i);
    }
  MLOG_INFO(mlog::app, "End Test Exceptions");
}

void* threadMain(void* arg){
  MLOG_INFO(mlog::app, "Thread says hello", DVAR(pthread_self()));
  return 0;
}

void test_pthreads(){
  MLOG_INFO(mlog::app, "Test Pthreads");
	pthread_t p;
 
	auto tmp = pthread_create(&p, NULL, &threadMain, NULL);
  MLOG_INFO(mlog::app, "pthread_create returned", DVAR(tmp));
	pthread_join(p, NULL);

  MLOG_INFO(mlog::app, "End Test Pthreads");
}

void test_omp(){
  MLOG_INFO(mlog::app, "Test Openmp");
  int nthreads, tid;
  omp_set_num_threads(2);
/* Fork a team of threads giving them their own copies of variables */
#pragma omp parallel private(nthreads, tid)
  {

  /* Obtain thread number */
  tid = omp_get_thread_num();
  MLOG_INFO(mlog::app, "Hello World from thread", DVAR(tid));

  /* Only master thread does this */
  if (tid == 0) 
    {
    nthreads = omp_get_num_threads();
  MLOG_INFO(mlog::app, "Number of threads", DVAR(nthreads));
    }

  }
  MLOG_INFO(mlog::app, "End Test Openmp");
}

void primeFactorization_omp(uint64_t number){
	MLOG_INFO(mlog::app, "do primeFactorization for", DVAR(number), "on", DVAR(omp_get_thread_num()));
	uint64_t num = number;
	uint64_t limit = static_cast<uint64_t>(sqrt(num));
	for(uint64_t factor=2; factor <= limit; ++factor){
		while((num%factor) == 0){
			MLOG_INFO(mlog::app,"primeFactor:",DVAR(number),DVAR(factor));
			num = num / factor;
			limit = static_cast<uint64_t>(sqrt(num));
		}
	}
	if(num>1){
		uint64_t factor = num;
		MLOG_INFO(mlog::app,"primeFactor:",DVAR(number),DVAR(factor));
	}
	MLOG_INFO(mlog::app, "primeFactorization for", DVAR(number), "on", DVAR(omp_get_thread_num()), "done");
}

void primeFactors_omp(){
	omp_set_num_threads(5);
	MLOG_INFO(mlog::app, "start primeFactors_omp");

	#pragma omp parallel sections
	{
		#pragma omp section
			primeFactorization_omp(uint64_t(9999991)*uint64_t(10000019));
		#pragma omp section
			primeFactorization_omp(uint64_t(1000003)*uint64_t(9999991));
		/*#pragma omp section
			primeFactorization_omp(uint64_t(999983)*uint64_t(1000003));
		#pragma omp section
			primeFactorization_omp(uint64_t(100003)*uint64_t(999983));
		#pragma omp section
			primeFactorization_omp(uint64_t(99991)*uint64_t(100003));
		#pragma omp section
			primeFactorization_omp(uint64_t(10007)*uint64_t(99991));
		#pragma omp section
			primeFactorization_omp(uint64_t(9973)*uint64_t(10007));
		#pragma omp section
			primeFactorization_omp(uint64_t(1009)*uint64_t(9973));
		#pragma omp section
			primeFactorization_omp(uint64_t(997)*uint64_t(1009));
		#pragma omp section
			primeFactorization_omp(uint64_t(101)*uint64_t(997));*/
	}

	MLOG_INFO(mlog::app, "end primeFactors_omp");
}

void producerConsumers_omp(){
	omp_set_num_threads(2);
	MLOG_INFO(mlog::app, "start producerConsumers_omp");

	constexpr unsigned NUMBER = 100;

	volatile unsigned unconsumed = 0;
	volatile unsigned consumed = 0;

	#pragma omp parallel
	{
		//produce
		#pragma omp single nowait
		{
			MLOG_INFO(mlog::app, DVAR(omp_get_thread_num()), "start producing ...");
			for(unsigned i = 0; i<NUMBER; ++i){
				#pragma omp critical (unconsumed_lock)
					++unconsumed;
			}
			MLOG_INFO(mlog::app, DVAR(omp_get_thread_num()), "That is enough for now.", DVAR(NUMBER));
		}
		//consume
		MLOG_INFO(mlog::app,DVAR(omp_get_thread_num()),"start consuming ...");
		unsigned consumed_copy = 0;
		unsigned consumedLocal = 0;
		do {
			bool gotSomething = false;
			#pragma omp critical (unconsumed_lock)
			{
				if(unconsumed>0){
					--unconsumed;
					gotSomething = true;
				}
			}
			if(gotSomething){
				++consumedLocal;
				if(consumedLocal%(NUMBER/10) == 0)
					MLOG_INFO(mlog::app, DVAR(omp_get_thread_num()), DVAR(consumedLocal));
				#pragma omp critical (consumed_lock)
					++consumed;
			}
			#pragma omp critical (consumed_lock)
				consumed_copy = consumed;
		} while(consumed_copy < NUMBER);
		MLOG_INFO(mlog::app, DVAR(omp_get_thread_num()), "final count:", DVAR(consumedLocal));
	}

	MLOG_INFO(mlog::app, "end producerConsumer_omp");
}

mythos::Mutex mutex;
void* thread_main(void* ctx)
{
  MLOG_INFO(mlog::app, "hello thread!", DVAR(ctx));
  mutex << [ctx]() {
    MLOG_INFO(mlog::app, "thread in mutex", DVAR(ctx));
  };
  mythos::ISysretHandler::handle(mythos::syscall_wait());
  MLOG_INFO(mlog::app, "thread resumed from wait", DVAR(ctx));
  return 0;
}

void test_ExecutionContext()
{
  MLOG_INFO(mlog::app, "Test ExecutionContext");
  mythos::ExecutionContext ec1(capAlloc());
  mythos::ExecutionContext ec2(capAlloc());
  {
    MLOG_INFO(mlog::app, "test_EC: create ec1");
    mythos::PortalLock pl(portal); // future access will fail if the portal is in use already

    auto tls1 = mythos::setupNewTLS();
    ASSERT(tls1 != nullptr);
    auto res1 = ec1.create(kmem).as(myAS).cs(myCS).sched(mythos::init::SCHEDULERS_START)
    .prepareStack(thread1stack_top).startFun(&thread_main, nullptr, ec1.cap())
    .suspended(false).fs(tls1)
    .invokeVia(pl).wait();
    TEST(res1);

    MLOG_INFO(mlog::app, "test_EC: create ec2");
    auto tls2 = mythos::setupNewTLS();
    ASSERT(tls2 != nullptr);
    auto res2 = ec2.create(kmem).as(myAS).cs(myCS).sched(mythos::init::SCHEDULERS_START+1)
    .prepareStack(thread2stack_top).startFun(&thread_main, nullptr, ec2.cap())
    .suspended(false).fs(tls2)
    .invokeVia(pl).wait();
    TEST(res2);
  }

  for (volatile int i=0; i<100000; i++) {
    for (volatile int j=0; j<1000; j++) {}
  }

  MLOG_INFO(mlog::app, "sending notifications");
  mythos::syscall_signal(ec1.cap());
  mythos::syscall_signal(ec2.cap());
  MLOG_INFO(mlog::app, "End Test ExecutionContext");
}

void test_InterruptControl() {
  MLOG_INFO(mlog::app, "test_InterruptControl start");
  mythos::InterruptControl ic(mythos::init::INTERRUPT_CONTROL_START);

  mythos::PortalLock pl(portal); // future access will fail if the portal is in use already

  mythos::ExecutionContext ec(capAlloc());
  auto tls = mythos::setupNewTLS();
  ASSERT(tls != nullptr);
  auto res1 = ec.create(kmem).as(myAS).cs(myCS).sched(mythos::init::SCHEDULERS_START + 2)
    .prepareStack(thread3stack_top).startFun(&thread_main, nullptr, ec.cap())
    .suspended(false).fs(tls)
    .invokeVia(pl).wait();
  TEST(res1);
  TEST(ic.registerForInterrupt(pl, ec.cap(), 0x32).wait());
  TEST(ic.unregisterInterrupt(pl, 0x32).wait());
  TEST(capAlloc.free(ec, pl));
  MLOG_INFO(mlog::app, "test_InterruptControl end");
}


int main()
{
  char const str[] = "hello world!";
  mythos::syscall_debug(str, sizeof(str)-1);
  MLOG_ERROR(mlog::app, "application is starting :)", DVARhex(msg_ptr), DVARhex(initstack_top));

	{
		mythos::PortalLock pl(portal);
		pl.invoke<mythos::protocol::PerformanceMonitoring::InitializeCounters>(mythos::init::PERFORMANCE_MONITORING_MODULE);
	}

  /*test_float();
  test_Example();
  test_Portal();
  test_memory_root();*/
  test_heap(); // heap must be initialized for tls test
  /*test_tls();
  test_exceptions();
  //test_InterruptControl();
  //test_HostChannel(portal, 24*1024*1024, 2*1024*1024);
  test_ExecutionContext();
  test_pthreads();
  test_omp();*/

	{
		mythos::PortalLock pl(portal);
		pl.invoke<mythos::protocol::PerformanceMonitoring::CollectValues>(mythos::init::PERFORMANCE_MONITORING_MODULE);
	}

	primeFactors_omp();
	//producerConsumers_omp();

	{
		mythos::PortalLock pl(portal);
		pl.invoke<mythos::protocol::PerformanceMonitoring::MeasureSpeedup>(mythos::init::PERFORMANCE_MONITORING_MODULE);
	}

	/*{
		mythos::PortalLock pl(portal);
		pl.invoke<mythos::protocol::PerformanceMonitoring::MeasureCollectLatency>(mythos::init::PERFORMANCE_MONITORING_MODULE);
	}*/

	{
		mythos::PortalLock pl(portal);
		pl.invoke<mythos::protocol::PerformanceMonitoring::PrintValues>(mythos::init::PERFORMANCE_MONITORING_MODULE);
	}

  char const end[] = "bye, cruel world!";
  mythos::syscall_debug(end, sizeof(end)-1);

  return 0;
}
