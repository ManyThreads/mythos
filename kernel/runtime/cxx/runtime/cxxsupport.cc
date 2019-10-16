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

#include <cstddef>
#include <cstdint>
#include <stdlib.h>
#include <cstdarg>
#include <endian.h>
#include <pthread.h>
#include <atomic>
#include <sys/types.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/time.h>

#include "mythos/syscall.hh"
#include "runtime/mlog.hh"
#include "util/elf64.hh"
#include "runtime/ExecutionContext.hh"
#include "runtime/Portal.hh"
#include "runtime/ExecutionContext.hh"
#include "runtime/CapMap.hh"
#include "runtime/Example.hh"
#include "runtime/PageMap.hh"
#include "runtime/KernelMemory.hh"
#include "runtime/SimpleCapAlloc.hh"
#include "runtime/tls.hh"
#include "runtime/futex.hh"

extern mythos::InvocationBuf* msg_ptr asm("msg_ptr");
extern mythos::Portal portal;
extern mythos::CapMap myCS;
extern mythos::PageMap myAS;
extern mythos::KernelMemory kmem;
extern mythos::SimpleCapAllocDel capAlloc;

extern "C" [[noreturn]] void __assert_fail (const char *expr, const char *file, int line, const char *func)
{
    mlog::Logger<> logassert("assert");
    logassert.error("ASSERT",expr,"failed in",file,":",line,func);
    mythos::syscall_exit(-1); /// @TODO syscall_abort(); to see some stack backtrace etc
}

extern "C" long mythos_musl_syscall(long num, long a1, long a2, long a3,
	             long a4, long a5, long a6)
{
    MLOG_DETAIL(mlog::app, "mythos_musl_syscall", DVAR(num), 
            DVAR(a1), DVAR(a2), DVAR(a3),
            DVAR(a4), DVAR(a5), DVAR(a6));
    // see http://blog.rchapman.org/posts/Linux_System_Call_Table_for_x86_64/
    switch (num) {
    case 60: // exit(exit_code)
        MLOG_DETAIL(mlog::app, "syscall exit");
        asm volatile ("syscall" : : "D"(0), "S"(a1) : "memory");
        return 0;
    case 202: // sys_futex
        MLOG_DETAIL(mlog::app, "syscall futex");
        {
            uint32_t val2 = 0;
            return do_futex(reinterpret_cast<uint32_t*>(a1) /*uaddr*/, a2 /*op*/, a3 /*val*/,   nullptr/* timeout*/, nullptr /*uaddr2*/, val2/*val2*/, a6/*val3*/);
        }
    case 231: // exit_group for all pthreads 
        MLOG_DETAIL(mlog::app, "syscall exit_group");
        return -1;
    default:
        /*MLOG_ERROR(mlog::app, "mythos_musl_syscall", DVAR(num), 
            DVAR(a1), DVAR(a2), DVAR(a3),
            DVAR(a4), DVAR(a5), DVAR(a6));*/
        MLOG_ERROR(mlog::app, "Error: mythos_musl_syscall NYI", DVAR(num));
    }
    return -1;
}

extern "C" void * mmap(void *start, size_t len, int prot, int flags, int fd, off_t off){
    MLOG_DETAIL(mlog::app, "mmap");
	if(fd == -1){
		return malloc(len);
	}

	errno = ENOMEM;
	return MAP_FAILED;
}

extern "C" int munmap(void *start, size_t len){
    MLOG_DETAIL(mlog::app, "munmap");
	return 0;
}

extern "C" int unmapself(void *start, size_t len){
    MLOG_DETAIL(mlog::app, "unmapself");
    while(1);
	return 0;
}

extern "C" int mprotect(void *addr, size_t len, int prot){
    MLOG_DETAIL(mlog::app, "mprotect");
	//size_t start, end;
	//start = (size_t)addr & -PAGE_SIZE;
	//end = (size_t)((char *)addr + len + PAGE_SIZE-1) & -PAGE_SIZE;
	return 0;
}

void* testStart(void* bla){
    MLOG_DETAIL(mlog::app, "testStart");
    return bla;
}

struct StartStruct{
    int (*func)(void*);
    void* arg;
    mythos::CapPtr ec;
};

//void* startFun(void* arg){
    //auto start = reinterpret_cast<StartStruct*>(arg);
    //mythos::localEC = start->ec;
    //int (*func)(void*) = start->func;
    //void* args = start->arg;
    //delete start;
    //func(args);
    //return 0;
//}

int myclone(int (*func)(void *), void *stack, int flags, void *arg, int* ptid, void* tls, int ctid){
    MLOG_DETAIL(mlog::app, "myclone");
    mythos::PortalLock pl(portal); // future access will fail if the portal is in use already

    mythos::ExecutionContext ec(capAlloc());

    //auto start = new StartStruct;
    //start->func = func;
    //start->arg = arg; 
    //start->ec = ec.cap();
    //auto tls = mythos::setupNewTLS();
    ASSERT(tls != nullptr);
    auto res1 = ec.create(kmem).as(myAS).cs(myCS).sched(mythos::init::SCHEDULERS_START + 1)
    .prepareStack(stack).startFunInt(func, arg, ec.cap())
    .suspended(false).fs(tls)
    .invokeVia(pl).wait();

    //return -ENOSYS;
    MLOG_DETAIL(mlog::app, DVAR(ec.cap()));
    return ec.cap();
}

extern "C" int clone(int (*func)(void *), void *stack, int flags, void *arg,...){
    MLOG_DETAIL(mlog::app, "clone wrapper");
	va_list args;
	va_start(args, arg);
	int* ptid = va_arg(args, int*);
	void* tls = va_arg(args, void*);
	int ctid = va_arg(args, int);
	va_end(args);

	return myclone(func, stack, flags, arg, ptid, tls, ctid);
}

struct dl_phdr_info
{
    void* dlpi_addr; /* Base address of object */
    const char* dlpi_name; /* (Null-terminated) name of object */
    const void* dlpi_phdr; /* Pointer to array of ELF program headers for this object */
    uint16_t dlpi_phnum; /* # of items in dlpi_phdr */
};


extern char __executable_start; //< provided by the default linker script

/** walk through list of shared objects.
 * http://man7.org/linux/man-pages/man3/dl_iterate_phdr.3.html
 * https://github.com/ensc/dietlibc/blob/master/libcruft/dl_iterate_phdr.c
 * 
 * WARNING: use --eh-frame-hdr linker flag to ensure the presence of the EH_FRAME segment!
 */
extern "C" int dl_iterate_phdr(
    int (*callback) (dl_phdr_info *info, size_t size, void *data), void *data)
{
    MLOG_ERROR(mlog::app, "dl_iterate_phdr", DVAR((void*)callback), DVAR(&__executable_start));
    mythos::elf64::Elf64Image img(&__executable_start);
    ASSERT(img.isValid());

    // HACK this assumes llvm libunwind
    // the targetAddr contains the instruction pointer where the exception was thrown
    struct dl_iterate_cb_data {
        void *addressSpace;
        void *sects;
        void *targetAddr;
    };
    auto cbdata = static_cast<dl_iterate_cb_data *>(data);
    MLOG_ERROR(mlog::app, "dl_iterate_phdr", DVAR(cbdata->addressSpace),
        DVAR(cbdata->sects), DVAR(cbdata->targetAddr));
 
    dl_phdr_info info;
    info.dlpi_addr = 0; // callback ignores this header if its target addr is smaller than this.
    info.dlpi_name = "init.elf";
    info.dlpi_phdr = (const void*)img.phdr(0);
    info.dlpi_phnum = img.phnum();
    auto res = (*callback) (&info, sizeof(info), data);
    return res;
}

