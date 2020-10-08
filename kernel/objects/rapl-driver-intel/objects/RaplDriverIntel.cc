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

    dram_avail = 0;
    pp0_avail = 0;
    pp1_avail = 0;
    psys_avail = 0;
    different_units = 0;

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
          pp0_avail=1;
          pp1_avail=0;
          dram_avail=1;
          different_units=0;
          psys_avail=0;
          break;

        case CPU_HASWELL_EP:
        case CPU_BROADWELL_EP:
        case CPU_SKYLAKE_X:
          pp0_avail=1;
          pp1_avail=0;
          dram_avail=1;
          different_units=1;
          psys_avail=0;
          break;

        case CPU_KNIGHTS_LANDING:
        case CPU_KNIGHTS_MILL:
          pp0_avail=0;
          pp1_avail=0;
          dram_avail=1;
          different_units=1;
          psys_avail=0;
          break;

        case CPU_SANDYBRIDGE:
        case CPU_IVYBRIDGE:
          pp0_avail=1;
          pp1_avail=1;
          dram_avail=0;
          different_units=0;
          psys_avail=0;
          break;

        case CPU_HASWELL:
        case CPU_HASWELL_ULT:
        case CPU_HASWELL_GT3E:
        case CPU_BROADWELL:
        case CPU_BROADWELL_GT3E:
        case CPU_ATOM_GOLDMONT:
        case CPU_ATOM_GEMINI_LAKE:
        case CPU_ATOM_DENVERTON:
          pp0_avail=1;
          pp1_avail=1;
          dram_avail=1;
          different_units=0;
          psys_avail=0;
          break;

        case CPU_SKYLAKE:
        case CPU_SKYLAKE_HS:
        case CPU_KABYLAKE:
        case CPU_KABYLAKE_MOBILE:
        //case CPU_COFFEELAKE:
        //case CPU_COFFEELAKE_U:
          pp0_avail=1;
          pp1_avail=1;
          dram_avail=1;
          different_units=0;
          psys_avail=1;
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

    power_units = bits(x86::getMSR(MSR_RAPL_POWER_UNIT),3,0);
    uint32_t pu = x86::getMSR(MSR_RAPL_POWER_UNIT);
    MLOG_ERROR(mlog::boot, "power_units", pu);
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

    // calc

    if(pp0_avail){
      //auto status = getMSR(MSR_PP0_ENERGY_STATUS);
    }
    if(pp1_avail){
    }
    if(dram_avail){
    }
    if(psys_avail){
    }


  }

  //Error RaplDriverIntel::invoke_setInitMem(Tasklet*, Cap, IInvocation* msg)
  //{
    //auto data = msg->getMessage()->cast<protocol::CpuDriverKNC::SetInitMem>();

    ///// @todo should be implemented similar to the CapRef to invocation buffer in Portals
    //// however: revoking the capability does not prevent the host from continuing access

    //TypedCap<IFrame> frame(msg->lookupEntry(data->capPtrs[0]));
    //if (!frame) return frame;
    //auto info = frame.getFrameInfo();
    //if (info.device || !info.writable) return Error::INVALID_CAPABILITY;

    //MLOG_INFO(mlog::boot, "invoke setInitMem", DVAR(data->capPtrs[0]),
              //DVARhex(info.start.physint()), DVAR(info.size));

    //auto hostInfoPtr = PhysPtr<PhysPtr<HostInfoTable>>::fromPhys(&hostInfoPtrPhys);
    //auto hostInfo = *hostInfoPtr;
    //if (!hostInfo) return Error::INSUFFICIENT_RESOURCES;
    //hostInfo->initMem = info.start.physint();
    //hostInfo->initMemSize = info.size;

    //return Error::SUCCESS;
  //}

} // namespace mythos
