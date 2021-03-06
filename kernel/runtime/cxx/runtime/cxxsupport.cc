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
#include <sys/resource.h>
#include <bits/alltypes.h>

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
#include "runtime/ProcessorAllocator.hh"
#include "runtime/CapAlloc.hh"
#include "runtime/tls.hh"
#include "runtime/futex.hh"
#include "runtime/umem.hh"
#include "runtime/thread-extra.hh"
#include "mythos/InfoFrame.hh"

extern mythos::InfoFrame* info_ptr asm("info_ptr");
extern mythos::Portal portal;
extern mythos::CapMap myCS;
extern mythos::PageMap myAS;
extern mythos::KernelMemory kmem;
extern mythos::ProcessorAllocator pa;

// synchronization for pthread deletion (exit/join)
struct PthreadCleaner{
  PthreadCleaner()
    : flag(FREE)
  {
    //MLOG_ERROR(mlog::app, "PthreadCleaner");
  }

  enum state{
    UNKNOWN = 0, // invalid
    FREE = 1, // initial state
    EXITED = 2 // target pthread has exited and it is now save to free its memory and EC
    // otherwise it holds the waiters EC pointer
  };

  // marks the target pthread as finished (does not access its memory/stack anymore)
  // called by the finished pthread after pthread_exit and just before syscall_exit
  void exit(){
    //MLOG_DETAIL(mlog::app, "PthreadCleaner exit", DVARhex(this), DVARhex(pthread_self()));
    auto ec = flag.exchange(EXITED);
    if(ec != FREE){
      ASSERT(ec!=UNKNOWN);
      // wake waiter EC
      mythos::syscall_signal(ec);
    }
  }

  // wait until pthread t has finished (called exit())
  // when returning from this function, it is save to free the target pthreads memory and EC
  void wait(pthread_t t){
    auto pcs = reinterpret_cast<PthreadCleaner* >(t - (pthread_self() - reinterpret_cast<uintptr_t>(this)));
    //MLOG_DETAIL(mlog::app, "PthreadCleaner wait", DVARhex(pcs), DVARhex(this), DVARhex(pthread_self()), DVARhex(t));
    while(pcs->flag.load() != EXITED){
      mythos::CapPtr exp = FREE;
      // try to register as waiter
      if(pcs->flag.compare_exchange_weak(exp, mythos_get_pthread_ec_self())){
        //MLOG_DETAIL(mlog::app, "PthreadCleaner going to wait");
        mythos_wait();
      }
    }
  }
  
  // lock
  std::atomic<mythos::CapPtr> flag;
};

static thread_local PthreadCleaner pthreadCleaner;

extern "C" [[noreturn]] void __assert_fail (const char *expr, const char *file, int line, const char *func)
{
    mlog::Logger<> logassert("assert");
    logassert.error("ASSERT",expr,"failed in",file,":",line,func);
    mythos::syscall_exit(-1); /// @TODO syscall_abort(); to see some stack backtrace etc
}

void mythosExit(){
    MLOG_ERROR(mlog::app, "MYTHOS:PLEASE KILL ME!!!!!!1 elf");
}

struct iovec
{
    const char* io_base;
    size_t iov_len;
};

ssize_t writev(int fd, const struct iovec *iov, int iovcnt)
{
    //MLOG_WARN(mlog::app, "syscall writev");
    ssize_t ret = 0;
    for (int i = 0; i < iovcnt; i++) {
        mlog::sink->write(iov[i].io_base, iov[i].iov_len);
        ret += iov[i].iov_len;
    }
    return ret;
}

int prlimit(
    pid_t pid, int resource, const struct rlimit *new_limit,
    struct rlimit *old_limit)
{
    // dummy implementation
    return 0;
}

int sched_setaffinity(pid_t pid, size_t cpusetsize, cpu_set_t *mask)
{
    //MLOG_DETAIL(mlog::app, "syscall sched_setaffinity", DVAR(pid), DVAR(cpusetsize), DVARhex(mask));
    if(cpusetsize == info_ptr->getNumThreads() && mask == NULL) return -EFAULT;
    return 0;
}

int sched_getaffinity(pid_t pid, size_t cpusetsize, cpu_set_t *mask)
{
    //MLOG_DETAIL(mlog::app, "syscall sched_getaffinity", DVAR(pid), DVAR(cpusetsize), DVARhex(mask));
    if (mask) {
        //CPU_ZERO(mask);
	memset(mask, 0, cpusetsize);
        for(int i = 0; i < info_ptr->getNumThreads(); i++) CPU_SET(i, mask);
    }
    return info_ptr->getNumThreads();
}

void clock_gettime(long clk, struct timespec *ts){
    unsigned low,high;
    asm volatile("rdtsc" : "=a" (low), "=d" (high));
    unsigned long tsc = low | uint64_t(high) << 32;	
        //MLOG_DETAIL(mlog::app, "syscall clock_gettime", DVAR(clk), DVARhex(ts), DVAR(tsc), DVAR((tsc * PS_PER_TSC)/1000000000000));
    ts->tv_nsec = (tsc * info_ptr->getPsPerTSC() / 1000)%1000000000;
    ts->tv_sec = (tsc * info_ptr->getPsPerTSC())/1000000000000;
}

extern "C" long mythos_musl_syscall(
    long num, long a1, long a2, long a3,
    long a4, long a5, long a6)
{
    //MLOG_DETAIL(mlog::app, "mythos_musl_syscall", DVAR(num), 
    //DVAR(a1), DVAR(a2), DVAR(a3),
    //DVAR(a4), DVAR(a5), DVAR(a6));
    // see http://blog.rchapman.org/posts/Linux_System_Call_Table_for_x86_64/
    switch (num) {
    case 9:  //mmap
        MLOG_WARN(mlog::app, "syscall mmap NYI please use stub");
        return 0;
    case 10:  //mprotect
        MLOG_WARN(mlog::app, "syscall mprotect NYI");
        return 0;
    case 11:  //munmap
        MLOG_WARN(mlog::app, "syscall munmap NYI please use stub!");
        return 0;
    case 12:  //brk
        //MLOG_WARN(mlog::app, "syscall brk NYI");
        return -1;
    case 13: // rt_sigaction
        //MLOG_WARN(mlog::app, "syscall rt_sigaction NYI");
        return 0;
    case 14: // rt_sigprocmask(how, set, oldset, sigsetsize)
        //MLOG_WARN(mlog::app, "syscall rt_sigprocmask NYI");
        return 0;
    case 16: // ioctl
        //MLOG_WARN(mlog::app, "syscall ioctl NYI");
        return 0;
    case 20: // writev(fd, *iov, iovcnt)
        //MLOG_ERROR(mlog::app, "syscall writev NYI");
        return writev(a1, reinterpret_cast<const struct iovec *>(a2), a3);
    case 24: // sched_yield
        //MLOG_ERROR(mlog::app, "syscall sched_yield NYI");
        return 0;
    case 28:  //madvise
        MLOG_WARN(mlog::app, "syscall madvise NYI");
        return 0;
    case 39: // getpid
        MLOG_WARN(mlog::app, "syscall getpid NYI");
        return 0;
    case 60: // exit(exit_code)
        //MLOG_ERROR(mlog::app, "syscall exit", DVAR(a1));
        pthreadCleaner.exit();        
        asm volatile ("syscall" : : "D"(0), "S"(a1) : "memory");
        return 0;
    case 186: // gettid
        return mythos_get_pthread_tid(pthread_self());
    case 200: // tkill(pid, sig)
        MLOG_WARN(mlog::app, "syscall tkill NYI");
        return 0;
    case 202: // sys_futex
        //MLOG_DETAIL(mlog::app, "syscall futex");
        {
        //MLOG_ERROR(mlog::app, "Error: syscall futex", DVAR(num), 
            //DVAR(a1), DVAR(a2), DVAR(a3),
            //DVAR(a4), DVAR(a5), DVAR(a6));
            uint32_t val2 = 0;
            return do_futex(reinterpret_cast<uint32_t*>(a1) /*uaddr*/, 
                            a2 /*op*/, a3 /*val*/, reinterpret_cast<uint32_t*>(a4)/* timeout*/, 
                            reinterpret_cast<uint32_t*>(a5) /*uaddr2*/, a4/*val2*/, a6/*val3*/);
        }
    case 203: // sched_setaffinity
        return sched_setaffinity(a1, a2, reinterpret_cast<cpu_set_t*>(a3));
    case 204: // sched_getaffinity
        return sched_getaffinity(a1, a2, reinterpret_cast<cpu_set_t*>(a3));
    case 228: // clock_gettime
        //MLOG_ERROR(mlog::app, "Error: mythos_musl_syscall clock_gettime", DVAR(num), 
            //DVARhex(a1), DVARhex(a2), DVARhex(a3),
            //DVARhex(a4), DVARhex(a5), DVARhex(a6));
	clock_gettime(a1, reinterpret_cast<struct timespec *>(a2));
        return 0;
    case 231: // exit_group for all pthreads 
        MLOG_WARN(mlog::app, "syscall exit_group NYI");
	mythosExit();
        return 0;
    case 302: // prlimit64
        //MLOG_WARN(mlog::app, "syscall prlimit64 NYI", DVAR(a1), DVAR(a2), DVAR(a3), DVAR(a4), DVAR(a5), DVAR(a6));
        return 1;
    default:
        MLOG_ERROR(mlog::app, "Error: mythos_musl_syscall NYI", DVAR(num), 
            DVAR(a1), DVAR(a2), DVAR(a3),
            DVAR(a4), DVAR(a5), DVAR(a6));
    }
    return -1;
}

extern "C" void * mmap(void *start, size_t len, int prot, int flags, int fd, off_t off)
{
    // dummy implementation
    //MLOG_DETAIL(mlog::app, "mmap", DVAR(start), DVAR(len), DVAR(prot), DVAR(prot), DVAR(flags), DVAR(fd), DVAR(off));
    auto tmp = mythos::heap.alloc(len, mythos::align4K);
    if (!tmp){ 
	    errno = ENOMEM;
	    return MAP_FAILED;
    }

    if (flags & MAP_ANONYMOUS)  {
        memset(reinterpret_cast<void*>(*tmp), 0, len);
    }

    return reinterpret_cast<void*>(*tmp);
}

extern "C" int munmap(void *start, size_t len)
{
    // dummy implementation
    //MLOG_DETAIL(mlog::app, "munmap", DVAR(start), DVAR(len));
    mythos::heap.free(reinterpret_cast<unsigned long>(start));
    return 0;
}

extern "C" int unmapself(void *start, size_t len)
{
    PANIC_MSG(false, "unmapself: NYI!");
    return 0;
}

extern "C" int mprotect(void *addr, size_t len, int prot)
{
    // dummy implementation
    //MLOG_DETAIL(mlog::app, "mprotect");
    //size_t start, end;
    //start = (size_t)addr & -PAGE_SIZE;
    //end = (size_t)((char *)addr + len + PAGE_SIZE-1) & -PAGE_SIZE;
    return 0;
}

int myclone(
    int (*func)(void *), void *stack, int flags, 
    void *arg, int* ptid, void* tls, int* ctid)
{
    //MLOG_DETAIL(mlog::app, "myclone");
    ASSERT(tls != nullptr);

    // The compiler expect a kinda strange alignment coming from clone:
    // -> rsp % 16 must be 8
    // You can see this also in musl/src/thread/x86_64/clone.s (rsi is stack)
    // We will use the same trick for alignment as musl libc
    auto rsp = (uintptr_t(stack) & uintptr_t(-16))-8;

    mythos::PortalLock pl(portal); // future access will fail if the portal is in use already
    mythos::ExecutionContext ec(capAlloc());
    if (ptid && (flags&CLONE_PARENT_SETTID)) *ptid = int(ec.cap());
    // @todo store thread-specific ctid pointer, which should set to 0 by the OS on the thread's exit

    auto sc = pa.alloc(pl).wait();
    ASSERT(sc);
    if(sc->cap == mythos::null_cap){
      MLOG_WARN(mlog::app, "Processor allocation failed!");
      //todo: set errno = EAGAIN
      return (-1);
    }

    auto res1 = ec.create(kmem)
      .as(myAS)
      .cs(myCS)
      .sched(sc->cap)
      .rawStack(rsp)
      .rawFun(func, arg)
      .suspended(false)
      .fs(tls)
      .invokeVia(pl)
      .wait();
    //MLOG_DETAIL(mlog::app, DVAR(ec.cap()));
    return ec.cap();
}

extern "C" int clone(int (*func)(void *), void *stack, int flags, void *arg, ...)
{
    //MLOG_DETAIL(mlog::app, "clone wrapper");
    va_list args;
    va_start(args, arg);
    int* ptid = va_arg(args, int*);
    void* tls = va_arg(args, void*);
    int* ctid = va_arg(args, int*);
    va_end(args);
    return myclone(func, stack, flags, arg, ptid, tls, ctid);
}

// synchronize and cleanup exited pthread
extern "C" void mythos_pthread_cleanup(pthread_t t){
    MLOG_DETAIL(mlog::app, "mythos_pthread_cleanup", mythos_get_pthread_ec(t));
    // wait for target pthread to exit
    pthreadCleaner.wait(t);
    // delete EC of target pthread
    auto cap = mythos_get_pthread_ec(t);
    mythos::PortalLock pl(portal); 
    capAlloc.free(cap, pl);
    // memory of target pthread will be free when returning from this function
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
