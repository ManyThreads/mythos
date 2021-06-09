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
 * Copyright 2021 Philipp Gypser, BTU Cottbus-Senftenberg
 */
#pragma once

#include "cpu/ctrlregs.hh"

namespace mythos {
  namespace cpu {

    inline bool sse3Supported() { return bits(x86::cpuid(1).ecx, 0); }
    inline bool mwaitSupported() { return bits(x86::cpuid(1).ecx, 3); }
    inline bool mwaitEnumExtensionsSupported() { return bits(x86::cpuid(5).ecx, 0); }
    inline unsigned mwaitSmallestLineSize() { return bits(x86::cpuid(5).eax, 15, 0); }
    inline unsigned mwaitLargestLineSize() { return bits(x86::cpuid(5).ebx, 15, 0); }

    static inline void clflush(volatile void *p) {
      asm volatile("clflush %0" : "+m" (*(volatile char*)p));
    }
    
    static inline void wbinvd() {
      asm volatile("wbinvd": : :"memory");
    }

    static inline void barrier() {
      asm volatile("" ::: "memory");
    }

    static inline void clearInt() {
      asm volatile("cli" :::);
    }

    static inline void pause() {
      asm volatile("pause" :::);
    }

    static inline void fence() {
      x86::cpuid(1);
      //asm volatile("mfence" :::);
    }

    inline bool invariantTSC() { return bits(x86::cpuid(0x80000007).edx, 8); }

    inline uint64_t PsPerTSC() {
      auto regs = x86::cpuid(0x15);
      ASSERT(regs.ebx);
      if(regs.ecx){
        return (regs.eax * 1000000000000) / (regs.ebx * regs.ecx);
      }else{
        return (regs.eax * 1000000) / (regs.ebx * 25); //assume 25MHz 
      }
    }

    inline uint64_t tscToPS(uint64_t tsc) {
      auto regs = x86::cpuid(0x15);
      ASSERT(regs.ebx);
      if(regs.ecx){
        return (tsc * regs.eax * 1000000000000) / (regs.ebx * regs.ecx);
      }else{
        return (tsc * regs.eax * 1000000) / (regs.ebx * 25); //assume 25MHz 
      }
    }

    static inline uint64_t getTSC() {
      unsigned low,high;
      asm volatile("rdtsc" : "=a" (low), "=d" (high));
      return low | uint64_t(high) << 32;
    }

    static inline void monitor(const void *addr, unsigned long extension,unsigned long hint) {
      //asm volatile(".byte 0x0f, 0x01, 0xc8;" :: "a" (addr), "c" (extension), "d"(hint));
      asm volatile("monitor" :: "a" (addr), "c" (extension), "d"(hint));
    }

    inline unsigned numCStates() { return 8; }
    inline unsigned numSubCStates(unsigned cState) { 
      ASSERT(cState < numCStates());
      return bits(x86::cpuid(5).edx, 3 + (4*cState), 0 + (4*cState)); 
    }

    static inline unsigned long mwaitHint(unsigned cState, unsigned subCState) { 
      if(cState > 0){
        return  ((cState - 1) & 0xf) << 4 | (subCState & 0xf); 
      }
      return  0xf << 4 | (subCState & 0xf); 
    }

    static inline void mwait(unsigned long hint, unsigned long extension) {
      //asm volatile(".byte 0x0f, 0x01, 0xc9;" :: "a" (hint), "c" (extension));
      asm volatile("mwait" :: "a" (hint), "c" (extension));
    }

  } // namespace cpu
} // namespace mythos
