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

void __attribute__((weak)) initMythos(){};

int wrapper_main(){
	initMythos();
	return main();
}

extern "C" void start_c(long *)
{
    // init TLS is done by musl libc
    //mythos::setupInitialTLS();

    // TODO make a fake environment structure, which would normally be provided by the process creator
    // at the bottom of the stack 
    // see http://dbp-consulting.com/tutorials/debugging/linuxProgramStartup.html
    // http://articles.manugarg.com/aboutelfauxiliaryvectors.html
    
    // when initialising the stack:
    // push end marker
    // push environment and command line strings
    // push padding
    // push AT_NULL terminator
    // push AUXV entries
    // push nullptr for termination of environment variables
    // push environment variables
    // push nullptr for command line termination
    // push command line arguments
    // push argc
    
    // need AT_PHDR, AT_PHNUM, AT_PHENT, 
    // maybe AT_RANDOM, AT_HWCAP, AT_SYSINFO, AT_PAGESZ, AT_SECURE
    
    // can add own entries with id>AUX_CNT and musl/libc will ignore these    
    // for example, pointer to capability environment information, which can also be placed on the stack 
    
    struct Auxv 
    {
        Auxv(uint64_t a_type, uint64_t a_val) : a_type(a_type), a_val(a_val) {}
        uint64_t a_type;              /* Entry type */
        uint64_t a_val;           /* Integer value */
    };
    
    mythos::elf64::Elf64Image img(&__executable_start);

    struct ProgEnv 
    {
        ProgEnv() {}
        long argc = 0;
        // here the pointers to command line arguments would come, terminated by nullptr in addition to argc
        char* argvEnd = nullptr;
        // here the pointers to environment variables would come, terminated by nullptr
        char* envEnd = nullptr;
        // here the AUX vector comes
        Auxv phdr={3, 0};
        Auxv phent={4, 0};
        Auxv phnum={5, 0};
        Auxv pagesize={6, 4096};
        Auxv secure={23, 1};
        Auxv auxvEnd={0,0};
    };
    
    ProgEnv env;
    long* p = (long*)&env;
    
    env.phdr.a_val = (uint64_t)img.phdr(0);
    env.phnum.a_val = img.phnum();
    env.phent.a_val = img.phent();
   
    int argc = p[0];
    char **argv = (char **)(p+1);
    __libc_start_main(&wrapper_main, argc, argv);
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
