/* -*- mode:C++; -*- */
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
 * Copyright 2014 Randolf Rotta, Maik Krüger, and contributors, BTU Cottbus-Senftenberg 
 */
#pragma once

#include <cstdint>

namespace mythos {

  /** helper functions to query the CPUID configuration data 
   * see also http://www.etallen.com/cpuid.html
   * https://software.intel.com/en-us/articles/intel-64-architecture-processor-topology-enumeration
   * http://malekb.free.fr/Detecting%20Multi-Core%20Processor%20Topology%20in%20an%20IA-32%20Platform.pdf
   * http://support.amd.com/TechDocs/25481.pdf
   */
  class X86cpuid {
  public:
    static uint32_t maxLeave() { return cpuid(0).eax; }

    static uint32_t initialApicID() { return bits(cpuid(1).ebx,31,24); }
    static uint32_t maxApicThreads() { return bits(cpuid(1).ebx,23,16); }
    static uint32_t maxApicCores() { return 1+bits(cpuid(4,0).eax,31,26); }
    
    // see http://developer.amd.com/resources/documentation-articles/articles-whitepapers/processor-and-core-enumeration-using-cpuid/
    // does not work on XeonPhi
    static uint32_t apicIdCoreIdSize() { return 1<<bits(cpuid(0x80000008).ecx,15,12); }

    static bool hasLeaveB() { return maxLeave()>=11 && cpuid(11,0).ebx!=0; }
    static uint32_t x2ApicID() { return cpuid(11, 0).edx; }
    static uint32_t x2ApicThreadsPerCore() { return 1<<bits(cpuid(11,0).eax,4,0); } 
    static uint32_t x2ApicThreadsPerPkg() { return 1<<bits(cpuid(11,1).eax,4,0); }
    static uint32_t x2ApicThreadsEnabled(unsigned level) { return bits(cpuid(11,level).ebx,15,0); }

    /** check if the 8-entry page-attribute table (PAT) is
     * supported. When the PAT is supported, three bits in certain
     * paging-structure entries select a memory type (used to
     * determine type of caching used) from the PAT (see Section
     * 4.9.2). 
     */
    static bool hasPAT() { return bits(cpuid(0x01).edx,16); }

    /** check if CR4.PCIDE may be set to 1, enabling process-context identifiers inside the TLB */
    static bool hasPCIDE() { return bits(cpuid(0x01).ecx,17); }

    /** check if CR4.SMEP may be set to 1, enabling supervisor-mode execution prevention */
    static bool hasSMEP() { return bits(cpuid(0x07).ebx,7); }

    /** check if IA32_EFER.NXE may be set to 1, allowing PAE paging
     * and IA-32e paging to disable execute access to selected
     * pages */
    static bool hasNXE() { return bits(cpuid(0x80000001).edx,20); }

    /** check if 1-GByte pages are supported with IA-32e paging */
    static bool has1Gpages() { return bits(cpuid(0x80000001).edx,26); }

  public:
    struct Regs {
      uint32_t eax;
      uint32_t ebx;
      uint32_t ecx;
      uint32_t edx;
    };

    /** query the processor with the cpuid instruction */
    static Regs cpuid(uint32_t eax, uint32_t ecx=0) {
      Regs r;
      asm volatile("cpuid"
		   : "=a"(r.eax), "=b"(r.ebx), "=c"(r.ecx), "=d"(r.edx)
		   : "a"(eax), "c"(ecx));
      return r;
    }

    /** extract a bitrange from an integer */
    static constexpr uint32_t bits(uint32_t val, unsigned last, unsigned first) {
      return (val >> first) & ((1<<(last-first+1))-1);
    }

    /** extract a single bit from an integer */
    static constexpr bool bits(uint32_t val, unsigned pos) {
      return (val >> pos) & 0x1;
    }
  };

} // namespace mythos
