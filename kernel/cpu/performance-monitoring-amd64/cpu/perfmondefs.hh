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
 * Copyright 2021 Philipp Gypser and Florian Bartz, BTU Cottbus-Senftenberg
 */
#pragma once

#include "cpu/ctrlregs.hh"
#include "util/bitfield.hh"

#include <cstdint>
#include <cstddef>

namespace mythos {
  namespace x86 {
    constexpr size_t MAX_NUM_FIXED_CTRS = 3;
    constexpr size_t MAX_NUM_PMCS = 4;
    constexpr size_t MAX_NUM_OFFCORE_RSP_MSRS = 2;

    /* architectural performance monitoring events availability bits in EBX using cpuid */
    enum PerfMonEventAvail{
      // Core cycle event not available if 1
      CORE_CYCLE_EVENT_AVAIL = 0,
      // Instruction retired event not available if 1
      INSTR_RETIRED_EVENT_AVAIL = 1,
      // Reference cycles event not available if 1
      REF_CYCLES_EVENT_AVAIL = 2,
      // Last-level cache reference event not available if 1
      LL_CACHE_REF_EVENT_AVAIL = 3,
      // Last-level cache misses event not available if 1
      LL_CACHE_MISS_EVENT_AVAIL = 4,
      // Branch instruction retired event not available if 1
      BRANCH_INSTR_RETIRED_EVENT_AVAIL = 5,
      // Branch mispredict retired event not available if 1
      BRANCH_MISPRED_RETIRED_EVENT_AVAIL = 6,
      // Top-down slots event not available if 1
      TOP_DOWN_SLOTS_EVENT_AVAIL = 7
    };

    /* Performance Event Select Register */
		BITFIELD_DEF(uint64_t, IA32_FIXED_CTR_CTRL_Bitfield)
		BoolField<value_t,base_t,0> ff0_enable_os; // fixed function counter 0 enable OS
		BoolField<value_t,base_t,1> ff0_enable_user; 
		BoolField<value_t,base_t,2> ff0_anyThread; // count events of all threads on this core
		BoolField<value_t,base_t,3> ff0_pmi; // Enable PMI on overflow
		BoolField<value_t,base_t,4> ff1_enable_os;
		BoolField<value_t,base_t,5> ff1_enable_user;
		BoolField<value_t,base_t,6> ff1_anyThread;
		BoolField<value_t,base_t,7> ff1_pmi; // Enable PMI on overflow
		BoolField<value_t,base_t,8> ff2_enable_os;
		BoolField<value_t,base_t,9> ff2_enable_user;
		BoolField<value_t,base_t,10> ff2_anyThread;
		BoolField<value_t,base_t,11> ff2_pmi; // Enable PMI on overflow
    IA32_FIXED_CTR_CTRL_Bitfield() : value(0) {}
    BITFIELD_END

    /* Configure Off-core Response Events */
		BITFIELD_DEF(uint64_t, MSR_OFFCORE_RSPx_Bitfield)
    //Counts the number of demand and DCU prefetch data reads of full and partial 
    //cachelines as well as demand data page table entry cacheline reads. Does not 
    //count L2 data read prefetches or instruction fetches.
		BoolField<value_t,base_t,0> dmnd_data_rd; 
    //Counts the number of demand and DCU prefetch reads for ownership (RFO) 
    //requests generated by a write to data cacheline. Does not count L2 RFO.
		BoolField<value_t,base_t,1> dmnd_rfo; 
    //Counts the number of demand instruction cacheline reads and L1 instruction 
    //cacheline prefetches.
		BoolField<value_t,base_t,2> dmnd_ifetch; 
    //Counts the number of writeback (modified to exclusive) transactions.
		BoolField<value_t,base_t,3> wb; 
    //Counts the number of data cacheline reads generated by L2 prefetchers.
		BoolField<value_t,base_t,4> pf_data_rd; 
    //Counts the number of RFO requests generated by L2 prefetchers.
		BoolField<value_t,base_t,5> pf_rfo; 
    //Counts the number of code reads generated by L2 prefetchers.
		BoolField<value_t,base_t,6> pf_ifetch; 
    //Counts one of the following transaction types, including L3 invalidate, I/O, 
    //full or partial writes, WC or non-temporal stores, CLFLUSH, Fences, lock, 
    //unlock, split lock.
		BoolField<value_t,base_t,7> other; 
    //L3 Hit: local or remote home requests that hit L3 cache in the uncore with no 
    //coherency actions required (snooping).
		BoolField<value_t,base_t,8> uncore_hit; 
    //L3 Hit: local or remote home requests that hit L3 cache in the uncore and was 
    //serviced by another core with a cross core snoop where no modified copies were 
    //found (clean).
		BoolField<value_t,base_t,9> other_core_hit_snp; 
    //L3 Hit: local or remote home requests that hit L3 cache in the uncore and was 
    //serviced by another core with a cross core snoop where modified copies were 
    //found (HITM).
		BoolField<value_t,base_t,10> other_core_hitm; 
		/*BoolField<value_t,base_t,11> reserved;*/ 
    //L3 Miss: local homed requests that missed the L3 cache and was serviced by 
    //forwarded data following a cross package snoop where no modified copies found. 
    //(Remote home requests are not counted)
		BoolField<value_t,base_t,12> remote_cache_fwd; 
    //L3 Miss: remote home requests that missed the L3 cache and were serviced by 
    //remote DRAM.
		BoolField<value_t,base_t,13> remote_dram; 
    //L3 Miss: local home requests that missed the L3 cache and were serviced by 
    //local DRAM.
		BoolField<value_t,base_t,14> local_dram; 
    //Non-DRAM requests that were serviced by IOH.
		BoolField<value_t,base_t,15> non_dram; 
    MSR_OFFCORE_RSPx_Bitfield() : value(0) {}
		BITFIELD_END

    /* Performance Event Select Register */
		BITFIELD_DEF(uint64_t, IA32_PERFEVTSELx_Bitfield)
    //Event Select: Selects a performance event logic unit.
		UIntField<value_t,base_t,0,8> eventSelect;
    //UMask: Qualifies the microarchitectural
    //condition to detect on the selected event logic.
		UIntField<value_t,base_t,8,8> uMask;
    //USR: Counts while in privilege level is not ring 0.
		BoolField<value_t,base_t,16> usr;
    //OS: Counts while in privilege level is ring 0.
		BoolField<value_t,base_t,17> os;
    //Edge: Enables edge detection if set.
		BoolField<value_t,base_t,18> edge;
    //PC: Enables pin control.
		BoolField<value_t,base_t,19> pc;
    //INT: Enables interrupt on counter overflow.
		BoolField<value_t,base_t,20> intEn;
    //AnyThread: When set to 1, it enables counting the associated event 
    //conditions occurring across all logical processors sharing a processor 
    //core. When set to 0, the counter only increments the associated event 
    //conditions occurring in the logical processor which programmed the MSR.
		BoolField<value_t,base_t,21> anyThread;
    //EN: Enables the corresponding performance counter to commence 
    //counting when this bit is set.
		BoolField<value_t,base_t,22> en;
    //INV: Invert the CMASK.
		BoolField<value_t,base_t,23> invert;
    //CMASK: When CMASK is not zero, the corresponding performance counter
    //increments each cycle if the event count is greater than or equal to the CMASK.
		UIntField<value_t,base_t,24,8> cMask; // counter mask
    IA32_PERFEVTSELx_Bitfield() : value(0) {}
		BITFIELD_END

    //todo: check availability (see PerfMonEventAvail above)
    /* Architectural Performance Monitoring Events */
    //Introduced in Intel Core Solo and Intel Core Duo processors. They are also 
    //supportedon processors basedon Intel Core microarchitecture.
    enum ArchitecturalPerformanceMonitoringEvents {
      //Counts core clock cycles whenever the logical processoris in C0 state 
      //(not halted).The frequency of this event varies with state transitionsin 
      //the core.
      UnhaltedCoreCycles_evSel = 0x3C,
      UnhaltedCoreCycles_umask = 0x00,
      //Counts at a fixed frequency whenever the logical processor is in C0 state
      //(not halted).
      UnhaltedReferenceCycles_evSel = 0x3C,
      UnhaltedReferenceCycles_umask = 0x01,
      //Counts when the last uop of an instruction retires.
      InstructionsRetired_evSel = 0xC0,
      InstructionsRetired_umask = 0x00,
      //Accesses to the LLC, in which the data is present(hit) or not present(miss).
      LLCReference_evSel = 0x2E,
      LLCReference_umask = 0x4F,
      //Accesses to the LLC in which the data is not present(miss)
      LLCMisses_evSel = 0x2E,
      LLCMisses_umask = 0x41,
      //Counts when the last uop of a branch instruction retires
      BranchInstructionRetired_evSel = 0xC4,
      BranchInstructionRetired_umask = 0x00,
      //Counts when the last uop of a branch instruction retires which corrected 
      //misprediction of the branch prediction hardware at execution time .
      BranchMissesRetired_evSel = 0xC5,
      BranchMissesRetired_umask = 0x00,
      /* INST_RETIRED.ANY -> use FIXED_CTR_0 */
      /* CPU_CLK_UNHALTED.* -> use FIXED_CTR_1 */
      /* CPU_CLK_UNHALTED.REF_TSC -> use FIXED_CTR_2 */
    };

    //todo: this enum is incomplete! please add required entries.
    //the availability of those entries highly depends on cpu generation/type 
    /* Performance Monitoring Events based on Skylake Microarchitecture */
    enum PerformanceMonitoringEvents{
      //Execution stalls while memory subsystem has an outstanding load.
      CycleActivityStallsMemAny_evSel = 0xA3, 
      CycleActivityStallsMemAny_umask = 0x14, 
      CycleActivityStallsMemAny_cmask = 20, 
    };

  } // namespace x86
} // namespace mythos
