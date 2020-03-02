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
 * Copyright 2016 Randolf Rotta, Robert Kuban, and contributors, BTU Cottbus-Senftenberg
 */

#include <cstdlib>
#include <cstddef>
#include <atomic>

#include "mythos/syscall.hh"
#include "runtime/mlog.hh"
#include "runtime/tls.hh"
#include "util/elf64.hh"


extern "C" typedef void (*func_t)();
extern "C" [[noreturn]] void _Exit(int exit_code);
extern "C" [[noreturn]] void exit(int exit_code=0);

// this stuff is needed when running without musl libc:
// extern func_t __preinit_array_start[] __attribute__((weak));
// extern func_t __preinit_array_end[] __attribute__((weak));
// extern func_t __init_array_start[] __attribute__((weak));
// extern func_t __init_array_end[] __attribute__((weak));
// extern func_t __fini_array_start[] __attribute__((weak));
// extern func_t __fini_array_end[] __attribute__((weak));
// func_t _ctors_start[] __attribute__((section(".ctors"))) = {0};
// func_t _dtors_start[] __attribute__((section(".dtors"))) = {0};
// extern "C" func_t _ctors_end[];
// extern "C" func_t _dtors_end[];
// extern "C" __attribute__((weak)) void _init();
// extern "C" __attribute__((weak)) void _fini();

extern char __executable_start; //< provided by the default linker script

extern "C" __attribute__((noreturn)) int __libc_start_main(int (*)(), int, char **);

extern "C" int main();

extern "C" void start_c(long *)
{
    // init TLS is done by musl libc
    //mythos::setupInitialTLS();

    struct Auxv 
    {
        Auxv(uint64_t a_type, uint64_t a_val) : a_type(a_type), a_val(a_val) {}
        uint64_t a_type;              /* Entry type */
        uint64_t a_val;           /* Integer value */
    };

    // Make a fake environment structure.
    // This would normally be provided by the process creator
    // at the bottom of the stack. 
    // But it can be anywhere, because we pass it by pointer to the musl libc entry point.
    // see http://dbp-consulting.com/tutorials/debugging/linuxProgramStartup.html
    // http://articles.manugarg.com/aboutelfauxiliaryvectors.html
    //
    // The musl libc needs AT_PHDR, AT_PHNUM, AT_PHENT, 
    // and maybe also AT_RANDOM, AT_HWCAP, AT_SYSINFO, AT_PAGESZ, AT_SECURE.
    // We can add own entries with id>AUX_CNT and musl/libc will ignore these    
    // for example, pointer to capability environment information.
    struct ProgEnv 
    {
        ProgEnv() {}
        long argc = 0;
        // here the pointers to command line arguments would come, terminated by nullptr in addition to argc
        char* argvEnd = nullptr; // termination of arg vector
        // here the pointers to environment variables would come, terminated by nullptr
        char* envEnd = nullptr; // termination of env vector
        // here the AUX vector comes
        Auxv phdr={3, 0};
        Auxv phent={4, 0};
        Auxv phnum={5, 0};
        Auxv pagesize={6, 4096};
        Auxv secure={23, 1};
        Auxv auxvEnd={0,0}; // AT_NULL termination of the aux vector, would be bottom of stack
    };
    
    mythos::elf64::Elf64Image img(&__executable_start);
    ProgEnv env;
    long* p = (long*)&env;
    
    env.phdr.a_val = (uint64_t)img.phdr(0);
    env.phnum.a_val = img.phnum();
    env.phent.a_val = img.phent();
    
    // implemented in musl libc: src/env/__libc_start_main.c
    // uses char **envp = argv+argc+1;
    __libc_start_main(&main, env.argc, &env.argvEnd-env.argc);
}

// see also http://stackoverflow.com/a/28981890 :(
// extern "C" void __libc_init_array()
// {
//   for (size_t i = 0; i < __preinit_array_end - __preinit_array_start; i++)
//     __preinit_array_start[i]();
//   _init();
//   for (size_t i = 0; i < __init_array_end - __init_array_start; i++) {
//     __init_array_start[i]();
//   }
// 
//   for (size_t i = 1; i < _ctors_end - _ctors_start; i++) {
//     // mlog::TextMsg<200> msg("", "init", i, (void*)_ctors_end[-i]);
//     // mythos::syscall_debug(msg.getData(), msg.getSize());
//     _ctors_end[-i]();
//   }
// }

// static void __libc_fini_array()
// {
//   for (size_t i = 1; i < _dtors_end - _dtors_start; i++) {
//     // mlog::TextMsg<200> msg("", "init", i, (void*)_dtors_end[i]);
//     // mythos::syscall_debug(msg.getData(), msg.getSize());
//     _dtors_end[i]();
//   }
//   for (size_t i = __fini_array_end - __fini_array_start; i > 0; i--)
//     __fini_array_start[i-1]();
//   _fini();
// }


// void _Exit(int exit_code)
// {
//   asm volatile ("syscall" : : "D"(0), "S"(exit_code) : "memory");
//   while(true);
// }

// void exit(int exit_code)
// {
// //  for (size_t i = 0; i < atexit_funcs_count; i++)
// //    if (atexit_funcs[i]) atexit_funcs[i]();
//   __libc_fini_array();
//   _Exit(exit_code);
// }
