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

#define NUM_CPUS (3)

extern "C" [[noreturn]] void __assert_fail (const char *expr, const char *file, int line, const char *func)
{
    mlog::Logger<> logassert("assert");
    logassert.error("ASSERT",expr,"failed in",file,":",line,func);
    mythos::syscall_exit(-1); /// @TODO syscall_abort(); to see some stack backtrace etc
}

struct iovec
{
    const char* io_base;
    size_t iov_len;
};

ssize_t writev(int fd, const struct iovec *iov, int iovcnt)
{
    MLOG_WARN(mlog::app, "syscall writev");
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
    MLOG_ERROR(mlog::app, "syscall sched_setaffinity", DVAR(pid), DVAR(cpusetsize), DVARhex(mask));
    if(cpusetsize == NUM_CPUS && mask == NULL) return -EFAULT;
    return 0;
}

int sched_getaffinity(pid_t pid, size_t cpusetsize, cpu_set_t *mask)
{
    MLOG_ERROR(mlog::app, "syscall sched_getaffinity", DVAR(pid), DVAR(cpusetsize), DVARhex(mask));
    if (mask) {
        CPU_ZERO(mask);
        for(int i = 0; i < NUM_CPUS; i++) CPU_SET(i, mask);
    }
    return NUM_CPUS;
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
    case 13: // rt_sigaction
        MLOG_ERROR(mlog::app, "syscall rt_sigaction NYI");
        return 0;
    case 14: // rt_sigprocmask(how, set, oldset, sigsetsize)
        MLOG_ERROR(mlog::app, "syscall rt_sigprocmask NYI");
        return 0;
    case 16: // ioctl
        MLOG_ERROR(mlog::app, "syscall ioctl NYI");
        return 0;
    case 20: // writev(fd, *iov, iovcnt)
        //MLOG_ERROR(mlog::app, "syscall writev NYI");
        return writev(a1, reinterpret_cast<const struct iovec *>(a2), a3);
    case 24: // sched_yield
        MLOG_ERROR(mlog::app, "syscall sched_yield NYI");
        return 0;
    case 39: // getpid
        MLOG_ERROR(mlog::app, "syscall getpid NYI");
        return 0;
    case 60: // exit(exit_code)
        MLOG_DETAIL(mlog::app, "syscall exit", DVAR(a1));
        asm volatile ("syscall" : : "D"(0), "S"(a1) : "memory");
        return 0;
    case 200: // tkill(pid, sig)
        MLOG_ERROR(mlog::app, "syscall tkill NYI");
        return 0;
    case 202: // sys_futex
        MLOG_DETAIL(mlog::app, "syscall futex");
        {
            uint32_t val2 = 0;
            return do_futex(reinterpret_cast<uint32_t*>(a1) /*uaddr*/, 
                            a2 /*op*/, a3 /*val*/, nullptr/* timeout*/, 
                            nullptr /*uaddr2*/, val2/*val2*/, a6/*val3*/);
        }
    case 203: // sched_setaffinity
        return sched_setaffinity(a1, a2, reinterpret_cast<cpu_set_t*>(a3));
    case 204: // sched_getaffinity
        return sched_getaffinity(a1, a2, reinterpret_cast<cpu_set_t*>(a3));
    case 228: // clock_gettime
        MLOG_ERROR(mlog::app, "syscall clock_gettime NYI");
        return 0;
    case 231: // exit_group for all pthreads 
        MLOG_ERROR(mlog::app, "syscall exit_group NYI");
        return 0;
    case 302: // prlimit64
        MLOG_ERROR(mlog::app, "syscall prlimit64 NYI", DVAR(a1), DVAR(a2), DVAR(a3), DVAR(a4), DVAR(a5), DVAR(a6));
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
    MLOG_DETAIL(mlog::app, "mmap");
    if (fd == -1) return malloc(len);
    errno = ENOMEM;
    return MAP_FAILED;
}

extern "C" int munmap(void *start, size_t len)
{
    // dummy implementation
    MLOG_DETAIL(mlog::app, "munmap");
    return 0;
}

extern "C" int unmapself(void *start, size_t len)
{
    // dummy implementation
    MLOG_DETAIL(mlog::app, "unmapself");
    while(1);
    return 0;
}

extern "C" int mprotect(void *addr, size_t len, int prot)
{
    // dummy implementation
    MLOG_DETAIL(mlog::app, "mprotect");
    //size_t start, end;
    //start = (size_t)addr & -PAGE_SIZE;
    //end = (size_t)((char *)addr + len + PAGE_SIZE-1) & -PAGE_SIZE;
    return 0;
}

int myclone(
    int (*func)(void *), void *stack, int flags, 
    void *arg, int* ptid, void* tls, int* ctid)
{
    MLOG_DETAIL(mlog::app, "myclone");
    ASSERT(tls != nullptr);

    mythos::PortalLock pl(portal); // future access will fail if the portal is in use already
    mythos::ExecutionContext ec(capAlloc());
    mythos::Frame stateFrame(capAlloc());
    auto res2 = stateFrame.create(pl, kmem, 4096, 4096).wait();
    if (ptid && (flags&CLONE_PARENT_SETTID)) *ptid = int(ec.cap());
    // @todo store thread-specific ctid pointer, which should be set to 0 by the OS on the thread's exit
    // @todo needs interaction with a process internal scheduler or core manager in order to figure out where to schedule the new thread
    auto res1 = ec.create(kmem).as(myAS).cs(myCS).sched(mythos::init::SCHEDULERS_START + 1)
        .state(stateFrame.cap(), 0)
        .prepareStack(stack)
        .startFunInt(func, arg, ec.cap())
        .suspended(false).fs(tls)
        .invokeVia(pl).wait();
    MLOG_DETAIL(mlog::app, DVAR(ec.cap()));
    return ec.cap();
}

extern "C" int clone(int (*func)(void *), void *stack, int flags, void *arg, ...)
{
    MLOG_DETAIL(mlog::app, "clone wrapper");
    va_list args;
    va_start(args, arg);
    int* ptid = va_arg(args, int*);
    void* tls = va_arg(args, void*);
    int* ctid = va_arg(args, int*);
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
