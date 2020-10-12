/* -*- mode:C++; indent-tabs-mode:nil; -*- */
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
 * Copyright 2020 Philipp Gypser and contributors, BTU Cottbus-Senftenberg
 */

#include "objects/RaplDriverIntel.hh"
#include "mythos/InvocationBuf.hh"
#include "util/assert.hh"
#include "boot/mlog.hh"
#include "mythos/protocol/RaplDriverIntel.hh"
#include "mythos/HostInfoTable.hh"
#include "objects/TypedCap.hh"
#include "cpu/ctrlregs.hh"
#include "objects/IntelRegs.hh"
//#include "objects/IFrame.hh"
//#include <cmath>


namespace mythos {

  extern PhysPtr<HostInfoTable> hostInfoPtrPhys SYMBOL("_host_info_ptr");

  RaplDriverIntel::RaplDriverIntel(){
    MLOG_ERROR(mlog::boot, "RAPL driver startet");
    // check it is an Intel cpu
    auto r = x86::cpuid(0);
    char str[] = "0123456789ab";
    *reinterpret_cast<uint32_t*>(&str[0]) = r.ebx;
    *reinterpret_cast<uint32_t*>(&str[4]) = r.edx;
    *reinterpret_cast<uint32_t*>(&str[8]) = r.ecx;
    
    isIntel = true;
    char intelStr[] = INTEL_STR;
    for(int i = 0; i < 12; i++){
      if(str[i] != intelStr[i]){
        isIntel = false;
        break;
      }
    }

    if(isIntel){
      MLOG_ERROR(mlog::boot, "Congratulations, you are the proud owner of an Intel CPU.");
    }else{
      MLOG_ERROR(mlog::boot, "Unsupported processor manufacturer -> ", str);
    }

    // check cpu family
    cpu_fam = bits(x86::cpuid(1).eax, 11, 8);
    MLOG_ERROR(mlog::boot, "CPU family =", cpu_fam);

    // check extended cpu model 
    cpu_model = (bits(x86::cpuid(1).eax, 19, 16) << 4) + bits(x86::cpuid(1).eax, 7, 4);
    MLOG_ERROR(mlog::boot, "CPU extended model =", cpu_model);

    dram_avail = false;
    pp0_avail = false;
    pp1_avail = false;
    psys_avail = false;
    different_units = false;

    power_units = 0;
    time_units = 0;
    cpu_energy_units = 0;
    dram_energy_units = 0;
    
    if(!isIntel){
      return;
    }
    
    if(cpu_fam == 6){
      switch(cpu_model){

        case CPU_SANDYBRIDGE_EP:
        case CPU_IVYBRIDGE_EP:
          pp0_avail=true;
          dram_avail=true;
          break;

        case CPU_HASWELL_EP:
        case CPU_BROADWELL_EP:
        case CPU_SKYLAKE_X:
          pp0_avail=true;
          dram_avail=true;
          different_units=true;
          break;

        case CPU_KNIGHTS_LANDING:
        case CPU_KNIGHTS_MILL:
          dram_avail=true;
          different_units=true;
          break;

        case CPU_SANDYBRIDGE:
        case CPU_IVYBRIDGE:
          pp0_avail=true;
          pp1_avail=true;
          break;

        case CPU_HASWELL:
        case CPU_HASWELL_ULT:
        case CPU_HASWELL_GT3E:
        case CPU_BROADWELL:
        case CPU_BROADWELL_GT3E:
        case CPU_ATOM_GOLDMONT:
        case CPU_ATOM_GEMINI_LAKE:
        case CPU_ATOM_DENVERTON:
          pp0_avail=true;
          pp1_avail=true;
          dram_avail=true;
          break;

        case CPU_SKYLAKE:
        case CPU_SKYLAKE_HS:
        case CPU_KABYLAKE:
        case CPU_KABYLAKE_MOBILE:
        //case CPU_COFFEELAKE:
        //case CPU_COFFEELAKE_U:
          pp0_avail=true;
          pp1_avail=true;
          dram_avail=true;
          psys_avail=true;
          break;
        default:
          MLOG_ERROR(mlog::boot, "Unknown CPU model :(");
      }
    
    }else if(cpu_fam == 11){
      switch(cpu_model){

        case CPU_KNIGHTS_CORNER:
          MLOG_ERROR(mlog::boot, "Please find out which RAPL sensors are supported for KNC...");
          break;
        default:
          MLOG_ERROR(mlog::boot, "Unknown CPU model :(");
      }
    }else{
      MLOG_ERROR(mlog::boot, "Unsupported processor family");
      return;
    }

    //determine power unit
    //power_units = std::pow(0.5, bits(x86::getMSR(MSR_RAPL_POWER_UNIT),3,0));
    power_units = bits(x86::getMSR(MSR_RAPL_POWER_UNIT),3,0);
    MLOG_ERROR(mlog::boot, "msr_rapl_power_units", power_units);

    //determine time unit
    //time_units = std::pow(0.5, bits(x86::getMSR(MSR_RAPL_POWER_UNIT),19,16));
    time_units = bits(x86::getMSR(MSR_RAPL_POWER_UNIT),19,16);
    MLOG_ERROR(mlog::boot, "time_units", time_units);

    //determine cpu energy unit
    //cpu_energy_units = std::pow(0.5, bits(x86::getMSR(MSR_RAPL_POWER_UNIT),12,8));
    cpu_energy_units = bits(x86::getMSR(MSR_RAPL_POWER_UNIT),12,8);
    MLOG_ERROR(mlog::boot, "cpu_energy_units", cpu_energy_units);

    if(different_units){
      //dram_energy_units = std::pow(0.5, 16.0);
      dram_energy_units = 16;
    }else{
      dram_energy_units = cpu_energy_units; 
    }
    MLOG_ERROR(mlog::boot, "dram_energy_units", dram_energy_units);
    
    printEnergy();
  }

  void RaplDriverIntel::invoke(Tasklet* t, Cap self, IInvocation* msg)
  {
    monitor.request(t, [=](Tasklet* t){
        Error err = Error::NOT_IMPLEMENTED;
        switch (msg->getProtocol()) {
        case protocol::RaplDriverIntel::proto:
          err = protocol::RaplDriverIntel::dispatchRequest(this, msg->getMethod(), t, self, msg);
          break;
        }
        if (err != Error::INHIBIT) {
          msg->replyResponse(err);
          monitor.requestDone();
        }
      } );
  }

  void RaplDriverIntel::printEnergy(){
    if(!isIntel) return;

    if(pp0_avail){
      uint64_t pp0_es = x86::getMSR(MSR_PP0_ENERGY_STATUS);
      MLOG_ERROR(mlog::boot, "Power plane 0 energy status =", pp0_es >> cpu_energy_units);
    }
    if(pp1_avail){
      uint64_t pp1_es = x86::getMSR(MSR_PP1_ENERGY_STATUS);
      MLOG_ERROR(mlog::boot, "Power plane 1 energy status =", pp1_es >> cpu_energy_units);
    }
    if(dram_avail){
      uint64_t dram_es = x86::getMSR(MSR_DRAM_ENERGY_STATUS);
      MLOG_ERROR(mlog::boot, "DRAM energy status =", dram_es >> dram_energy_units);
    }
    if(psys_avail){
      uint64_t pl_es = x86::getMSR(MSR_PLATFORM_ENERGY_STATUS);
      MLOG_ERROR(mlog::boot, "Platform energy status =", pl_es >> cpu_energy_units);
    }
  }

  Error RaplDriverIntel::invoke_getRaplVal(Tasklet*, Cap, IInvocation* msg)
  {
    MLOG_DETAIL(mlog::boot, __PRETTY_FUNCTION__);

    auto ret = msg->getMessage()->cast<protocol::RaplDriverIntel::Result>();

    ret->val.pp0 = 0;
    ret->val.pp1 = 0;
    ret->val.psys = 0;
    ret->val.dram = 0;
    ret->val.cpu_energy_units = cpu_energy_units;
    ret->val.dram_energy_units = dram_energy_units;

    if(pp0_avail){
      ret->val.pp0 = x86::getMSR(MSR_PP0_ENERGY_STATUS);
    }
    if(pp1_avail){
      ret->val.pp1 = x86::getMSR(MSR_PP1_ENERGY_STATUS);
    }
    if(dram_avail){
      ret->val.dram = x86::getMSR(MSR_DRAM_ENERGY_STATUS);
    }
    if(psys_avail){
      ret->val.psys = x86::getMSR(MSR_PLATFORM_ENERGY_STATUS);
    }

    return Error::SUCCESS;
  }

} // namespace mythos
