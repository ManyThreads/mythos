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
#include "cpu/perfmondefs.hh"
#include "objects/mlog.hh"
#include "cpu/LAPIC.hh"

#include <cstdint>
#include <cstddef>

namespace mythos {
  namespace x86 {

  class PerformanceMonitoring{
    public:
      PerformanceMonitoring()
        : perfMonLeaf({0, 0, 0, 0})
        , pmcs(0)
        , ffcs(0)
     {
        perfMonAvail = leafAvail();
        if(!perfMonAvail){
          MLOG_ERROR(mlog::perfmon, "Performance monitoring leaf not available in cpuid!");
        return;
        }
        perfMonLeaf = cpuid(ARCHITECTURAL_PERFORMANCE_MONTIROING_LEAF);

        pmcs = numGenPurpPerfMon();
        if(pmcs > MAX_NUM_PMCS){
          MLOG_WARN(mlog::perfmon, "Hardware supports more general purpose performance counter as used", DVAR(pmcs), DVAR(MAX_NUM_PMCS));
          pmcs = MAX_NUM_PMCS;
        }
        ffcs = numberFixedFunPCs();
        if(ffcs > MAX_NUM_FIXED_CTRS){
          MLOG_WARN(mlog::perfmon, "Hardware supports more fixed function performance counter as used", DVAR(ffcs), DVAR(MAX_NUM_FIXED_CTRS));
          ffcs = MAX_NUM_FIXED_CTRS;
        } 
      }

  /* features supported according to cpuid */  
      // Architectural Performance Monitoring Leaf available in CPUID
      bool leafAvail(){ return maxLeave() >= ARCHITECTURAL_PERFORMANCE_MONTIROING_LEAF; }

      // Version ID of architectural performance monitoring
      uint32_t version() { 
        ASSERT(perfMonAvail);
        return bits(perfMonLeaf.eax, 7, 0); 
      }

      // Number of general-purpose performance monitoring counter per logical processor
      uint32_t numGenPurpPerfMon() { 
        ASSERT(perfMonAvail);
        return bits(perfMonLeaf.eax, 15, 8); 
      }

      // Bit width of general-purpose, performance monitoring counter
      uint32_t genPurpPerfMonBitWidth() { 
        ASSERT(perfMonAvail);
        return bits(perfMonLeaf.eax, 23, 16); 
      }

      // Length of EBX bit vector to enumerate architectural performance monitoring events
      uint32_t perfMonEventVectorLength() { 
        ASSERT(perfMonAvail);
        return bits(perfMonLeaf.eax, 31, 24); 
      }

      // check whether event is available
      bool eventAvail(unsigned bit){
        ASSERT(perfMonAvail);
        return perfMonEventVectorLength() > bit ? 
          !bits(perfMonLeaf.ebx, bit) : false; 
      } 
  
      // Supported fixed counters bit mask. Fixed-function performance counter 'i' is supported if bit
      // ‘i’ is 1 (first counter index starts at zero). It is recommended to use the following logic to 
      // determine if a Fixed Counter is supported: FxCtr[i]_is_supported := ECX[i] || (EDX[4:0] > i); 
      bool fixedCounterSupported(uint8_t i){
        ASSERT(perfMonAvail);
        return  (bits(perfMonLeaf.ecx, 4, 0) > i) || bits(perfMonLeaf.ecx, i); 
      }

      // Number of fixed-function performance counters (if Version ID > 1)
      uint32_t numberFixedFunPCs(){
        ASSERT(perfMonAvail);
        return version() > 1 ? 
          bits(perfMonLeaf.edx, 4, 0) : 0; 
      }

      // Bit width of fixed-function performance counters (if Version ID > 1)
      uint32_t bitWidthFixedFunPCs(){
        ASSERT(perfMonAvail);
        return version() > 1 ? 
          bits(perfMonLeaf.edx, 12, 5) : 0; 
      }
  /* perf configuration */
      uint64_t readInstRetired(){
        return getMSR(IA32_PERF_FIXED_CTR0);
      }

      uint64_t readCyclesUnhalted(){
        return getMSR(IA32_PERF_FIXED_CTR1);
      }

      uint64_t readRefCyclesUnhalted(){
        return getMSR(IA32_PERF_FIXED_CTR2);
      }

      void resetAllFFCS(){
        setMSR(IA32_PERF_FIXED_CTR0, 0);
        setMSR(IA32_PERF_FIXED_CTR1, 0);
        setMSR(IA32_PERF_FIXED_CTR2, 0);
      }

      void activateAllFFCS(){
        //ffcs_ctrl.ff0_enable_os = true;
        //ffcs_ctrl.ff1_enable_os = true;
        //ffcs_ctrl.ff2_enable_os = true;
        ffcs_ctrl.ff0_enable_user = true;
        ffcs_ctrl.ff1_enable_user = true;
        ffcs_ctrl.ff2_enable_user = true;
      }

      void deactivateAllFFCS(){
        ffcs_ctrl.ff0_enable_os = false;
        ffcs_ctrl.ff1_enable_os = false;
        ffcs_ctrl.ff2_enable_os = false;
        ffcs_ctrl.ff0_enable_user = false;
        ffcs_ctrl.ff1_enable_user = false;
        ffcs_ctrl.ff2_enable_user = false;
      }

      void enableIntAllFFCS(){
        ffcs_ctrl.ff0_pmi = true;
        ffcs_ctrl.ff1_pmi = true;
        ffcs_ctrl.ff2_pmi = true;
      }

      void setFfcsCtrl(IA32_FIXED_CTR_CTRL_Bitfield bf){
        ffcs_ctrl = bf;
      }

      void writeFFCS_CTRL(){
        ASSERT(perfMonAvail);
        setMSR(IA32_FIXED_CTR_CTRL, ffcs_ctrl);    
      }

      void setPerfevtsel(uint32_t index, const IA32_PERFEVTSELx_Bitfield bf) {
        ASSERT(perfMonAvail);
        ASSERT(index < pmcs);
        perfevsel[index] = bf.value;
      }

      void writePerfevtsel(uint32_t index) {
        ASSERT(perfMonAvail);
        ASSERT(index < pmcs);
        setMSR(IA32_PERFEVTSEL0 + index, perfevsel[index]);    
      }

      uint64_t readPMC(uint32_t index) { 
        ASSERT(perfMonAvail);
        ASSERT(index < pmcs);
        return getMSR(IA32_PMC0 + index); 
      }

      void writePMC(uint32_t index, uint64_t val) { 
        ASSERT(perfMonAvail);
        ASSERT(index < pmcs);
        setMSR(IA32_PMC0 + index, val); 
      }

      void setOffcoreRsp(uint32_t index, MSR_OFFCORE_RSPx_Bitfield bf) {
        ASSERT(perfMonAvail);
        ASSERT(index < MAX_NUM_OFFCORE_RSP_MSRS);
        offcore_rsp[index] = bf.value;
      }

      void writeOffcoreRsp(uint32_t index) {
        ASSERT(perfMonAvail);
        ASSERT(index < MAX_NUM_OFFCORE_RSP_MSRS);
        setMSR(MSR_OFFCORE_RSP_0 + index, offcore_rsp[index]);    
      }

      inline void barrier(){
        asm volatile ("":::"memory");
        perfMonLeaf = cpuid(ARCHITECTURAL_PERFORMANCE_MONTIROING_LEAF);
        asm volatile ("":::"memory");
      }

      void enableIntLapic(){
        setMSR(X2Apic::REG_LVT_PERFCNT, X2Apic::RegLVT().vector(42).masked(0));
      }

    private:
      bool perfMonAvail;
      Regs perfMonLeaf; 

      uint32_t pmcs; //number of general purpose performance monitoring counters
      uint32_t ffcs; //number of fixed function counters

      IA32_FIXED_CTR_CTRL_Bitfield ffcs_ctrl;
      IA32_PERFEVTSELx_Bitfield perfevsel[MAX_NUM_PMCS];
      MSR_OFFCORE_RSPx_Bitfield offcore_rsp[MAX_NUM_OFFCORE_RSP_MSRS];

  };

  } // namespace x86
} // namespace mythos

