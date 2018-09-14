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

#include "mythos/syscall.hh"
#include "runtime/mlog.hh"

/** cause abnormal process termination.
 * Defined in stdlib.h
 */
extern "C" void abort() {
    while(true) mythos::syscall_exit(-1); 
}

struct FILE;

FILE* stdin  = (FILE*)0x47110000;
FILE* stdout = (FILE*)0x47110001;
FILE* stderr = (FILE*)0x47110002;

extern "C" int fflush(FILE* stream) {
    MLOG_ERROR(mlog::app, "fflush", DVAR(stream));
    return 0;
}

extern "C" int fprintf(FILE *stream, const char *format, ...) {
    MLOG_ERROR(mlog::app, "fprintf", DVAR(stream), DVAR(format));
    return 0;
}

extern "C" int snprintf(char *str, size_t size, const char *format, ...) {
    MLOG_ERROR(mlog::app, "snprintf", DVAR(str), DVAR(size), DVAR(format));
    str[0] = '\0';
    return 0;
}

extern "C" int vfprintf(FILE *stream, const char *format, va_list) {
    MLOG_ERROR(mlog::app, "vfprintf", DVAR(stream), DVAR(format));
    return 0;
}

extern "C" int fputc(int c, FILE *stream) {
    MLOG_ERROR(mlog::app, "fputc", DVAR(stream), DVAR(c));
    return 0;
}

extern "C" size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream) {
    MLOG_ERROR(mlog::app, "fwrite", DVAR(stream), DVAR(ptr), DVAR(size), DVAR(nmemb));
    return nmemb;
}

extern "C" char *getenv(const char *name) {
    MLOG_ERROR(mlog::app, "getenv", DVAR(name));
    return nullptr;
}


extern "C" int isupper(int c) {
  unsigned char x=c&0xff;
  return (x>='A' && x<='Z') || (x>=192 && x<=222 && x!=215);
}

extern "C" int isxdigit(int ch) {
    return (unsigned int)( ch         - '0') < 10u  || 
           (unsigned int)((ch | 0x20) - 'a') <  6u;
}


struct pthread_rwlock_t;

extern "C" int pthread_rwlock_rdlock(pthread_rwlock_t* rwlock) {
    MLOG_ERROR(mlog::app, "pthread_rwlock_rdlock", DVAR(rwlock));
    return 0;
}

extern "C" int pthread_rwlock_unlock(pthread_rwlock_t *rwlock) {
    MLOG_ERROR(mlog::app, "pthread_rwlock_unlock", DVAR(rwlock));
    return 0;
}

extern "C" int pthread_rwlock_wrlock(pthread_rwlock_t *rwlock) {
    MLOG_ERROR(mlog::app, "pthread_rwlock_wrlock", DVAR(rwlock));
    return 0;
}


struct dl_phdr_info;

/** walk through list of shared objects.
 * http://man7.org/linux/man-pages/man3/dl_iterate_phdr.3.html
 * https://github.com/ensc/dietlibc/blob/master/libcruft/dl_iterate_phdr.c
 */
extern "C" int dl_iterate_phdr(
    int (*callback) (dl_phdr_info *info, size_t size, void *data), void *data)
{
    MLOG_ERROR(mlog::app, "dl_iterate_phdr", DVAR((void*)callback));
    return 0;
}

// http://wiki.osdev.org/C%2B%2B
void* __dso_handle = 0;

struct Dl_info
{
    const char *dli_fname;  /* Pathname of shared object that contains address */
    void       *dli_fbase;  /* Base address at which shared object is loaded */
    const char *dli_sname;  /* Name of symbol whose definition overlaps addr */
    void       *dli_saddr;  /* Exact address of symbol named in dli_sname */
};

// used only for debugging by libcxxabi ???
/** translate address to symbolic information */
extern "C" int dladdr(void* addr, Dl_info* info) {
    MLOG_ERROR(mlog::app, "dladdr", DVAR(addr), DVAR(info));
	return 0;
}

// http://refspecs.linuxbase.org/LSB_3.1.1/LSB-Core-generic/LSB-Core-generic/baselib---cxa-atexit.html
// http://wiki.osdev.org/C%2B%2B
/** register a function to be called by exit or when a shared library is unloaded */
extern "C" int __cxa_atexit(void (*func)(void*), void* arg, void* dso_handle) {
    MLOG_ERROR(mlog::app, "atexit", DVAR((void*)func), DVAR(arg), DVAR(dso_handle));
    return 0; // success
}

// http://wiki.osdev.org/C%2B%2B
// http://refspecs.linuxbase.org/LSB_5.0.0/LSB-Core-generic/LSB-Core-generic/baselib---cxa-finalize.html
/** call destructors of global (or local static) C++ objects and
 * exit functions registered with atexit */
extern "C" void __cxa_finalize(void* d) { 
    MLOG_ERROR(mlog::app, "finalize", DVAR(d));
}

// http://wiki.osdev.org/C%2B%2B
// implemented by libcxxabi
// extern "C" void __cxa_pure_virtual() { while(true) mythos::syscall_exit(-1); }

// http://refspecs.linux-foundation.org/LSB_4.1.0/LSB-Core-generic/LSB-Core-generic/libc---stack-chk-fail-1.html
/** terminate a function in case of stack overflow. */
extern "C" void __stack_chk_fail() {
    /// @todo does this even make sense, when we already have a stack overflow?
    MLOG_ERROR(mlog::app, "stack_chk_fail");
    while(true) mythos::syscall_exit(-1); 
}

// implemented by libcxxabi
// http://mentorembedded.github.io/cxx-abi/abi.html#member-pointers
// http://wiki.osdev.org/C%2B%2B#Local_Static_Variables_.28GCC_Only.29
// http://www.opensource.apple.com/source/libcppabi/libcppabi-14/src/cxa_guard.cxx
// namespace __cxxabiv1
// {
//   /* guard variables */
// 
//   /* The ABI requires a 64-bit type.  */
//   __extension__ typedef uint64_t __guard __attribute__((mode(__DI__)));
// 
//   extern "C" int __cxa_guard_acquire (__guard *);
//   extern "C" void __cxa_guard_release (__guard *);
//   extern "C" void __cxa_guard_abort (__guard *);
// 
//   extern "C" int __cxa_guard_acquire (__guard *g)
//   {
//     return !*(char *)(g);
//   }
// 
//   extern "C" void __cxa_guard_release (__guard *g)
//   {
//     *(char *)g = 1;
//   }
// 
//   extern "C" void __cxa_guard_abort (__guard *)
//   {}
// }
