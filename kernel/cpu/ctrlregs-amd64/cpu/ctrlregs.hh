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
 * Copyright 2019 Randolf Rotta and contributors, BTU Cottbus-Senftenberg
 */
#pragma once

#include <cstdint>
#include <cstddef>

namespace mythos {
  /** extract a bitrange from an integer */
  inline constexpr uint32_t bits(uint32_t val, unsigned last, unsigned first) {
    return (val >> first) & ((1<<(last-first+1))-1);
  }

  /** extract a single bit from an integer */
  inline constexpr bool bits(uint32_t val, unsigned pos) {
    return (val >> pos) & 0x1;
  }

  /** extract a bitrange from an integer */
  inline constexpr uint64_t bits(uint64_t val, unsigned last, unsigned first) {
    return (val >> first) & ((1<<(last-first+1))-1);
  }

  /** extract a single bit from an integer */
  inline constexpr bool bits(uint64_t val, unsigned pos) {
    return (val >> pos) & 0x1;
  }

  namespace x86 {
    struct Regs {
      uint32_t eax;
      uint32_t ebx;
      uint32_t ecx;
      uint32_t edx;
    };

    /** query the processor with the cpuid instruction */
    inline Regs cpuid(uint32_t eax, uint32_t ecx=0) {
      Regs r;
      asm volatile("cpuid"
           : "=a"(r.eax), "=b"(r.ebx), "=c"(r.ecx), "=d"(r.edx)
           : "a"(eax), "c"(ecx));
      return r;
    }

    inline uint32_t maxLeave() { return cpuid(0).eax; }

    inline uint32_t initialApicID() { return bits(cpuid(1).ebx,31,24); }
    inline uint32_t maxApicThreads() { return bits(cpuid(1).ebx,23,16); }
    inline uint32_t maxApicCores() { return 1+bits(cpuid(4,0).eax,31,26); }

    // see http://developer.amd.com/resources/documentation-articles/articles-whitepapers/processor-and-core-enumeration-using-cpuid/
    // does not work on XeonPhi
    inline uint32_t apicIdCoreIdSize() { return 1<<bits(cpuid(0x80000008).ecx,15,12); }

    inline bool hasLeaveB() { return maxLeave()>=11 && cpuid(11,0).ebx!=0; }

    inline bool x2ApicSupported() { return bits(cpuid(1).ecx, 21); }
    inline uint32_t x2ApicID() { return cpuid(11, 0).edx; }
    inline uint32_t x2ApicThreadsPerCore() { return 1<<bits(cpuid(11,0).eax,4,0); }
    inline uint32_t x2ApicThreadsPerPkg() { return 1<<bits(cpuid(11,1).eax,4,0); }
    inline uint32_t x2ApicThreadsEnabled(unsigned level) { return bits(cpuid(11,level).ebx,15,0); }

    /** check if the 8-entry page-attribute table (PAT) is
     * supported. When the PAT is supported, three bits in certain
     * paging-structure entries select a memory type (used to
     * determine type of caching used) from the PAT (see Section
     * 4.9.2).
     */
    inline bool hasPAT() { return bits(cpuid(0x01).edx,16); }

    /** check if CR4.PCIDE may be set to 1, enabling process-context identifiers inside the TLB */
    inline bool hasPCIDE() { return bits(cpuid(0x01).ecx,17); }

    /** check if CR4.SMEP may be set to 1, enabling supervisor-mode execution prevention */
    inline bool hasSMEP() { return bits(cpuid(0x07).ebx,7); }

    /** check if IA32_EFER.NXE may be set to 1, allowing PAE paging
     * and IA-32e paging to disable execute access to selected
     * pages */
    inline bool hasNXE() { return bits(cpuid(0x80000001).edx,20); }

    /** check if 1-GByte pages are supported with IA-32e paging */
    inline bool has1Gpages() { return bits(cpuid(0x80000001).edx,26); }

    enum MSR {
      IA32_APIC_BASE_MSR         = 0x0000001B,
      MSR_IA32_SYSENTER_CS       = 0x00000174,
      MSR_IA32_SYSENTER_ESP      = 0x00000175,
      MSR_IA32_SYSENTER_EIP      = 0x00000176,
      MSR_RAPL_POWER_UNIT        = 0x00000606,
      MSR_PKG_RAPL_POWER_LIMIT	 = 0x00000610,
      MSR_PKG_ENERGY_STATUS	     = 0x00000611,
      MSR_PKG_PERF_STATUS	       = 0x00000613,
      MSR_PKG_POWER_INFO         = 0x00000614,
      MSR_PP0_POWER_LIMIT	       = 0x00000638,
      MSR_PP0_ENERGY_STATUS	     = 0x00000639,
      MSR_PP0_POLICY             = 0x0000063A,
      MSR_PP0_PERF_STATUS	       = 0x0000063B,
      MSR_PP1_POWER_LIMIT	       = 0x00000640,
      MSR_PP1_ENERGY_STATUS	     = 0x00000641,
      MSR_PP1_POLICY	           = 0x00000642,
      MSR_DRAM_POWER_LIMIT	     = 0x00000618,
      MSR_DRAM_ENERGY_STATUS	   = 0x00000619,
      MSR_DRAM_PERF_STATUS	     = 0x0000061B,
      MSR_DRAM_POWER_INFO	       = 0x0000061C,
      MSR_PLATFORM_ENERGY_STATUS = 0x0000064D,
      MSR_EFER                   = 0xc0000080,
      MSR_IA32_STAR              = 0xc0000081,
      MSR_IA32_LSTAR             = 0xc0000082,
      MSR_IA32_FMASK             = 0xc0000084,
      MSR_FS_BASE                = 0xc0000100,
      MSR_GS_BASE                = 0xc0000101,
      MSR_KERNEL_GS_BASE         = 0xc0000102,
    };

    enum Idx {
      MSR_SYSCALL_SEG = 32, //< bit index for kernel segment selector in STAR
      MSR_SYSRET_SEG = 48,  //< bit index for user segment selector in START
      EFER_SCE = 1 << 0,    //< syscall enable
      EFER_LME = 1 << 8,    //< IA-32e mode enable
      EFER_LMA = 1 << 10,   //< IA-32e mode active indicator bit
      EFER_NXE = 1 << 11,    //< Execute-disable bit enable
      X2APIC_ENABLED = 1ul << 10, //< bit index for x2APIC enabled in APIC_BASE_MSR
      XAPIC_ENABLED  = 1ul << 11, //< bit index for APIC enabled in APIC_BASE_MSR
      XAPIC_BSP = 1ul << 8 //< BootStrap Processor flag (read only)
    };

    inline uint64_t getMSR(uint32_t msr) {
      uint32_t h,l;
      asm volatile ("rdmsr" : "=a"(l), "=d"(h) : "c"(msr));
      return (uint64_t(h) << 32) | l;
    }

    inline void setMSR(uint32_t msr, uint64_t value) {
      asm volatile ("wrmsr" : : "a"(uint32_t(value)),
            "d"(uint32_t(value >> 32)), "c"(msr));
    }

    inline size_t getApicBase() { return (getMSR(IA32_APIC_BASE_MSR) & 0xFFFFFF000); }
    inline bool isApicBSP() { return getMSR(IA32_APIC_BASE_MSR) & XAPIC_BSP; }
    inline bool isApicEnabled() { return getMSR(IA32_APIC_BASE_MSR) & XAPIC_ENABLED; }
    inline bool isX2ApicEnabled() { return getMSR(IA32_APIC_BASE_MSR) & X2APIC_ENABLED; }
    inline void disableApic() { setMSR(IA32_APIC_BASE_MSR, getMSR(IA32_APIC_BASE_MSR) & uint64_t(~XAPIC_ENABLED) & uint64_t(~X2APIC_ENABLED)); }
    inline void enableApic() { setMSR(IA32_APIC_BASE_MSR, getMSR(IA32_APIC_BASE_MSR) | XAPIC_ENABLED); }
    inline void enableX2Apic() {
      setMSR(IA32_APIC_BASE_MSR, getMSR(IA32_APIC_BASE_MSR) | XAPIC_ENABLED | X2APIC_ENABLED);
    }

    inline void setStar(uint64_t kerncs, uint64_t usercs32) {
      setMSR(MSR_IA32_STAR, usercs32<<MSR_SYSRET_SEG | kerncs<<MSR_SYSCALL_SEG);
    }

    inline void setLStar(void (*entryptr)()) { setMSR(MSR_IA32_LSTAR, uint64_t(entryptr)); }

    enum FLAGS {
      FLAG_CF      = 1ul << 0,  // 0x000001
      FLAG_PF      = 1ul << 2,  // 0x000004
      FLAG_AF      = 1ul << 4,  // 0x000010
      FLAG_ZF      = 1ul << 6,  // 0x000040
      FLAG_SF      = 1ul << 7,  // 0x000080
      FLAG_TF      = 1ul << 8,  // 0x000100
      FLAG_IF      = 1ul << 9,  // 0x000200
      FLAG_DF      = 1ul << 10, // 0x000400
      FLAG_OF      = 1ul << 11, // 0x000800
      FLAG_IOPL    = 3ul << 12, // 0x003000
      FLAG_NT      = 1ul << 14, // 0x004000
      FLAG_RF      = 1ul << 16, // 0x010000
      FLAG_VM      = 1ul << 17, // 0x020000
      FLAG_AC      = 1ul << 18, // 0x040000
      FLAG_VIF     = 1ul << 19, // 0x080000
      FLAG_VIP     = 1ul << 20, // 0x100000
      FLAG_ID      = 1ul << 21  // 0x200000
    };

    inline void setFMask(uint32_t flags) { setMSR(MSR_IA32_FMASK, flags); }
    inline void enableSyscalls() { setMSR(MSR_EFER, getMSR(MSR_EFER) | EFER_SCE); }
    inline void setFSBase(uint64_t value) { setMSR(MSR_FS_BASE, value); }
    inline uint64_t readFSBase() { return getMSR(MSR_FS_BASE); }
    inline void setGSBase(uint64_t value) { setMSR(MSR_GS_BASE, value); }
    inline uint64_t readGSBase() { return getMSR(MSR_GS_BASE); }
    inline void setKernelGSBase(uint64_t value) { setMSR(MSR_KERNEL_GS_BASE, value); }
    inline uint64_t readKernelGSBase() { return getMSR(MSR_KERNEL_GS_BASE); }

    enum CR0Bits {
      PG = 1<<31, // Paging: If 1, enable paging and use the CR3 register, else disable paging
      CD = 1<<30, // Cache disable: Globally enables/disable the memory cache
      NW = 1<<29, // Not-write through: Globally enables/disable write-through caching
      AM = 1<<18, // Alignment mask: Alignment check enabled if AM set, AC flag (in EFLAGS register) set, and privilege level is 3
      WP = 1<<16, // Write protect: Determines whether the CPU can write to pages marked read-only when privilege level is 0
      NE = 1<<5,  // Numeric error: Enable internal x87 floating point error reporting when set, else enables PC style x87 error detection
      ET = 1<<4,  // Extension type: On the 386, whether external math coprocessor is 80287 or 80387
      TS = 1<<3,  // Task switched: Allows saving x87 task context upon a task switch only after x87 instruction used
      EM = 1<<2,  // Emulation: If set, no x87 floating point unit present, if clear, x87 FPU present
      MP = 1<<1,  // Monitor co-processor: Controls interaction of WAIT/FWAIT instructions with TS flag in CR0
      PE = 1<<0   // Protected Mode Enable: If 1, system is in protected mode, else real mode
    };

    inline size_t getCR0() {
      size_t res;
      asm volatile ("mov %%cr0, %0" : "=r"(res));
      return res;
    }

    inline void setCR0(size_t val) { asm volatile ("mov %0, %%cr0" : : "r"(val)); }

    /** Page Fault Linear Address (PFLA). When a page fault occurs,
     * the address the program attempted to access is stored in the
     * CR2 register.
     */
    inline size_t getCR2() {
      size_t res;
      asm volatile ("mov %%cr2, %0" : "=r"(res));
      return res;
    }

    /** the upper 20 bits of CR3 become the page directory base
     * register (PDBR), which stores the physical address of the first
     * page directory entry.
     */
    inline size_t getCR3() {
      size_t res;
      asm volatile ("mov %%cr3, %0" : "=r"(res));
      return res;
    }

    inline void setCR3(size_t val) { asm volatile ("mov %0, %%cr3" : : "r"(val)); }

    enum CR4Bits {
      SMAP = 1<<21, // Supervisor Mode Access Protection Enable: access of data in a higher ring generates a fault
      SMEP = 1<<20, // Supervisor Mode Execution Protection Enable: execution of code in a higher ring generates a fault
      OSXSAVE = 1<<18, // XSAVE and Processor Extended States Enable
      PCIDE = 1<<17, // PCID Enable: enables process-context identifiers (PCIDs) in the TLB
      SMXE = 1<<14, // Safer Mode Extensions Enable, see Trusted Execution Technology (TXT)
      VMXE = 1<<13, // Virtual Machine Extensions Enable, see Intel VT-x
      OSXMMEXCPT = 1<<10, // Operating System Support for Unmasked SIMD Floating-Point Exceptions   If set, enables unmasked SSE exception.
      OSFXSR = 1<<9, // Operating system support for FXSAVE and FXRSTOR instructions: enables SSE instructions and fast FPU save & restoreCPU_COFFEELAKE_U
      PCE = 1<<8, // Performance-Monitoring Counter enable: RDPMC can be executed at any privilege level, else RDPMC can only be used in ring 0
      PGE = 1<<7, // Page Global Enabled: address translations may be shared between address spaces
      MCE = 1<<6, // Machine Check Exception: enables machine check interrupts to occur
      PAE = 1<<5, // Physical Address Extension: translate to extended 36-bit or 48-bit physical addresses
      PSE = 1<<4, // Page Size Extension: page size is increased to 4 MiB (or 2 MiB with PAE set)
      DE = 1<<3,  // Debugging Extensions: enables debug register based breaks on I/O space access
      TSD = 1<<2, // Time Stamp Disable: RDTSC instruction can only be executed when in ring 0, otherwise RDTSC can be used at any privilege level
      PVI = 1<<1, // Protected-mode Virtual Interrupts: enables support for the virtual interrupt flag (VIF) in protected mode
      VME = 1<<0  // Virtual 8086 Mode Extensions: enables support for the virtual interrupt flag (VIF) in virtual-8086 mode
    };

    inline size_t getCR4() {
      size_t res;
      asm volatile ("mov %%cr4, %0" : "=r"(res));
      return res;
    }

    inline void setCR4(size_t val) { asm volatile ("mov %0, %%cr4" : : "r"(val)); }

    // get CPU family
    inline uint32_t getCpuFam() { return bits(cpuid(1).eax, 11, 8); }
    // get CPU extended model
    inline uint32_t getCpuExtModel() { return (bits(cpuid(1).eax, 19, 16) << 4) + bits(cpuid(1).eax, 7, 4); }

    /* 
     * Intel Processor family 11 model number
     */
    enum IntelExtendedModelFamily11 {
      CPU_KNIGHTS_CORNER = 1
    };

    /* 
     * Intel Processor family 6 model number
     */
    enum IntelExtendedModelFamily6 {
      CPU_SANDYBRIDGE	=	42,
      CPU_SANDYBRIDGE_EP =	45,
      CPU_IVYBRIDGE	=	58,
      CPU_IVYBRIDGE_EP	= 62,
      CPU_HASWELL	=	60,
      CPU_HASWELL_ULT	= 69,
      CPU_HASWELL_GT3E	= 70,
      CPU_HASWELL_EP	=	63,
      CPU_BROADWELL	=	61,
      CPU_BROADWELL_GT3E	= 71,
      CPU_BROADWELL_EP	= 79,
      CPU_BROADWELL_DE	= 86,
      CPU_SKYLAKE	=	78,
      CPU_SKYLAKE_HS	=	94,
      CPU_SKYLAKE_X	 =	85,
      CPU_KNIGHTS_LANDING	= 87,
      CPU_KNIGHTS_MILL	= 133,
      CPU_KABYLAKE_MOBILE	= 142, // also CPU_COFFEELAKE_U
      CPU_KABYLAKE	=	158, // also CPU_COFFEELAKE
      CPU_ATOM_SILVERMONT	= 55,
      CPU_ATOM_AIRMONT	= 76,
      CPU_ATOM_MERRIFIELD	= 74,
      CPU_ATOM_MOOREFIELD	= 90,
      CPU_ATOM_GOLDMONT	= 92,
      CPU_ATOM_GEMINI_LAKE	= 122,
      CPU_ATOM_DENVERTON	= 95
    };
  } // namespace x86

  class IOPort8 {
    public:
      IOPort8 (unsigned port)
      : port(port)
      {}

      void write(uint8_t val){
        asm volatile("outb %0, %1" : : "a" (val), "Nd" (port));
      }

      uint8_t read(){
        uint8_t ret;
        asm volatile("inb %1, %0" : "=a" (ret) : "Nd" (port));
        return ret;
      }

    private:
      uint16_t port;
  };

  class IOPort16 {
    public:
      IOPort16 (unsigned port)
      : port(port)
      {}

      void write(uint16_t val){
        asm volatile("outw %0, %1" : : "a" (val), "Nd" (port));
      }

      uint16_t read(){
        uint16_t ret;
        asm volatile("inw %1, %0" : "=a" (ret) : "Nd" (port));
        return ret;
      }

    private:
      uint16_t port;
  };

  namespace cpu {
    /** Volatile isn't enough to prevent the compiler from reordering the
     * read/write functions for the control registers and messing everything up.
     * A memory clobber would solve the problem, but would prevent reordering of
     * all loads stores around it, which can hurt performance. Solution is to
     * use a variable and mimic reads and writes to it to enforce serialization
     */
    extern unsigned long __force_order;

    inline void loadPageTable(uintptr_t pdt_phys) {
      asm volatile("mov %0,%%cr3": : "r" (pdt_phys), "m" (__force_order));
    }

    inline uintptr_t getPageTable() {
      uintptr_t val;
      asm volatile("mov %%cr3,%0\n\t" : "=r" (val), "=m" (__force_order));
      return val;
    }

    inline void flushTLB() { loadPageTable(getPageTable()); }

    inline void flushTLB(void *log_addr) {
      asm volatile("invlpg (%0)" ::"r" (log_addr) : "memory");
    }
  } // namespace cpu

} // namespace mythos
