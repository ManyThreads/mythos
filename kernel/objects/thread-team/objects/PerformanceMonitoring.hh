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
 * Copyright 2021 Philipp Gypser, BTU Cottbus-Senftenberg
 */
#pragma once

#include "cpu/perfmon.hh"
#include "cpu/hwthreadid.hh"
#include "util/align.hh"
#include "util/assert.hh"
#include "objects/mlog.hh"

namespace mythos {
  namespace perfmon{

    struct alignas(alignLine) PerfMonMeasurement{
      void reset(){
        refCyclesUnhalted = 0;
        tscTicks = 0;
      }

      uint64_t refCyclesUnhalted = 0;
      uint64_t tscTicks = 0;
    };

    class alignas(alignLine) SchedulingContextMonitor{
      public:
        SchedulingContextMonitor(){
          ASSERT(pm.leafAvail());
        }

        void initCounters(){
          MLOG_ERROR(mlog::pm, __PRETTY_FUNCTION__);
          pm.activateAllFFCS();
          pm.enableIntAllFFCS();
          pm.writeFFCS_CTRL();
          resetCounters();
          pm.enableIntLapic();
        }

        void resetCounters(){
          //MLOG_ERROR(mlog::pm, __PRETTY_FUNCTION__);
          pm.barrier();
          timeStamp = getTSC();
          pm.resetAllFFCS(); 
          pm.barrier();
        }
       
        uint64_t getTSC(){
          unsigned low,high;
          asm volatile("rdtsc" : "=a" (low), "=d" (high));
          return low | uint64_t(high) << 32;	
        } 
        
        void print(){
          pm.barrier();
          auto now = getTSC();
          auto unhalted = pm.readRefCyclesUnhalted();
          pm.barrier();
          auto ticks = now-timeStamp;
          auto halted = ticks - unhalted;
          auto utilization = (unhalted * 100) / ticks;
          MLOG_ERROR(mlog::pm, DVAR(unhalted), DVAR(ticks), DVAR(halted), DVAR(utilization));
        }

        void read(PerfMonMeasurement* pmm){
          ASSERT(pmm);
          pm.barrier();
          auto now = getTSC();
          auto unhalted = pm.readRefCyclesUnhalted();
          pm.barrier();

          pmm->tscTicks = now - timeStamp;
          pmm->refCyclesUnhalted = unhalted;
        }
      private:
        x86::PerformanceMonitoring pm;
        uint64_t timeStamp;
    };

    extern SchedulingContextMonitor schedulingContextMonitor[];

    void initAt(cpu::ThreadID id){
      //MLOG_ERROR(mlog::pm, __PRETTY_FUNCTION__, DVAR(id));
      synchronousAt(async::getPlace(id)) << [=]() {
        schedulingContextMonitor[id].initCounters();
      }; 
    }

    void printAt(cpu::ThreadID id){
      //MLOG_ERROR(mlog::pm, __PRETTY_FUNCTION__, DVAR(id));
      synchronousAt(async::getPlace(id)) << [=]() {
        schedulingContextMonitor[id].print();
      }; 
    }

    void resetAt(cpu::ThreadID id){
      //MLOG_ERROR(mlog::pm, __PRETTY_FUNCTION__, DVAR(id));
      synchronousAt(async::getPlace(id)) << [=]() {
        schedulingContextMonitor[id].resetCounters();
      }; 
    }

    void readAt(cpu::ThreadID id, PerfMonMeasurement* pmm){
      //MLOG_ERROR(mlog::pm, __PRETTY_FUNCTION__, DVAR(id));
      synchronousAt(async::getPlace(id)) << [=]() {
        schedulingContextMonitor[id].read(pmm);
      }; 
    }
  } // namespace perfmon
} // namespace mythos
