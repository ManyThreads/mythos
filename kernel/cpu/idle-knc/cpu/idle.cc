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
 * Copyright 2016 Randolf Rotta, Robert Kuban, Maik Krüger, and contributors, BTU Cottbus-Senftenberg
 */

#include "cpu/idle.hh"
#include "util/Logger.hh"
#include "util/mstring.hh" // for memcpy
#include "boot/memory-layout.h" // for KNC MMIO
#include "cpu/ctrlregs.hh"
#include "cpu/hwthreadid.hh"

#define SBOX_C6_SCRATCH0 0x0000C000
#define MSR_CC6_STATUS 0x342

extern char _setup_ap_cc6;
extern char _setup_ap_end_cc6;

namespace mlog {
#ifndef MLOG_IDLE
#define MLOG_IDLE FilterError
#endif
static mlog::Logger<MLOG_IDLE> idle;
};

namespace mythos {
  namespace idle {

    CoreState coreStates[61];

    void init_global()
    {
      // copy CC6 trampoline to the right place
      memcpy(PhysPtr<char>(0x60000).log(),
       physPtr(&_setup_ap_cc6).log(),
       size_t(&_setup_ap_end_cc6 - &_setup_ap_cc6));
      asm volatile("wbinvd");

      /// @todo set CC6_EIP MSR?

      // turn on caches during re-entry from CC6 sleep
      auto cc6 = reinterpret_cast<uint32_t volatile*>(MMIO_ADDR+SBOX_BASE+SBOX_C6_SCRATCH0);
      *cc6 |= 0x8000; // C1-CC6 MAS (bit 15)

      for (unsigned i=0; i<22; i++)
        MLOG_DETAIL(mlog::idle, "C6_SCRATCH", DVAR(i), DVARhex(cc6[i]));
    }

    NORETURN void cpu_idle_halt() SYMBOL("cpu_idle_halt");

    void sleep()
    {
      size_t apicID = cpu::getThreadID(); // @todo hack on KNC because threadID==apicID
      while (coreStates[apicID/4].lock.exchange(true) == true);

      auto prev = coreStates[apicID/4].cc6ready.fetch_or(uint8_t(1 << (apicID%4)));
      /// @todo is this really needed if we always go into cc6?
      if ((prev | (1 << (apicID%4))) == 0xf) { // enable cc6
        MLOG_DETAIL(mlog::idle, "enable CC6", DVARhex(prev), DVAR(apicID));
        x86::setMSR(MSR_CC6_STATUS, x86::getMSR(MSR_CC6_STATUS) | 0x1f);
      }

      MLOG_DETAIL(mlog::idle, DVARhex(x86::getMSR(MSR_CC6_STATUS)));
      auto chlt = reinterpret_cast<uint64_t volatile*>(MMIO_ADDR+SBOX_BASE+0xac0c);
      MLOG_INFO(mlog::idle, "cores halted", DVARhex(*chlt));

      coreStates[apicID/4].lock = false;
      cpu_idle_halt();
    }

    void wokeup(size_t /*apicID*/, size_t reason)
    {
      MLOG_DETAIL(mlog::idle, DVARhex(x86::getMSR(MSR_CC6_STATUS)));
      if (reason == 1) {
        MLOG_INFO(mlog::idle, "woke up from CC6");
        cpu_idle_halt(); // woke up from CC6 => just sleep again
      }
    }

    void wokeupFromInterrupt()
    {
      MLOG_INFO(mlog::idle, "woke up from irq");
      size_t apicID = cpu::getThreadID(); // @todo hack on KNC because threadID==apicID
      while (coreStates[apicID/4].lock.exchange(true) == true);

      auto prev = coreStates[apicID/4].cc6ready.fetch_and(uint8_t(~(1u << (apicID%4))));
      if (prev == 0xf) { // disable cc6
        MLOG_DETAIL(mlog::idle, "disable CC6", DVARhex(prev), DVAR(apicID));
        x86::setMSR(MSR_CC6_STATUS, x86::getMSR(MSR_CC6_STATUS) & (~0x1Ful));
      }

      coreStates[apicID/4].lock = false;
    }

  } // namespace idle
} // namespace mythos
