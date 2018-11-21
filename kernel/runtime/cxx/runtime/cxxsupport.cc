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
#include <cstdarg>
#include <endian.h>
#include <pthread.h>
#include <atomic>

#include "mythos/syscall.hh"
#include "runtime/mlog.hh"
#include "util/elf64.hh"


extern "C" [[noreturn]] void __assert_fail (const char *expr, const char *file, int line, const char *func)
{
    mlog::Logger<> logassert("assert");
    logassert.error("ASSERT",expr,"failed in",file,":",line,func);
    mythos::syscall_exit(-1); /// @TODO syscall_abort(); to see some stack backtrace etc
}

extern "C" long mythos_musl_syscall(long num, long a1, long a2, long a3,
	             long a4, long a5, long a6)
{
    // see http://blog.rchapman.org/posts/Linux_System_Call_Table_for_x86_64/
    switch (num) {
    case 60: // exit(exit_code)
        asm volatile ("syscall" : : "D"(0), "S"(a1) : "memory");
        return 0;
    case 231: // exit_group for all pthreads 
        return -1;
    default:
        MLOG_ERROR(mlog::app, "mythos_musl_syscall", DVAR(num), 
            DVAR(a1), DVAR(a2), DVAR(a3),
            DVAR(a4), DVAR(a5), DVAR(a6));
    }
    return -1;
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

