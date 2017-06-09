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
#include "util/mstring.hh" // for memcpy
#include "boot/memory-layout.h" // for KNC MMIO
#include "cpu/ctrlregs.hh"

#define SBOX_C6_SCRATCH0 0x0000C000
#define MSR_CC6_STATUS 0x342

extern char _setup_ap_cc6;
extern char _setup_ap_end_cc6;

namespace mythos {
  namespace idle {

    CoreState coreStates[61];
    //CoreLocal<CoreState*> coreState KERNEL_CLM;

    void init_global()
    {
      // copy CC6 trampoline to the right place
      memcpy(PhysPtr<char>(0x60000).log(),
	     physPtr(&_setup_ap_cc6).log(),
	     size_t(&_setup_ap_end_cc6 - &_setup_ap_cc6));
      asm volatile("wbinvd");

      // copy PC6 trampoline to the right place
      // memcpy(PhysPtr<char>(0x70000).log(),
      //        physPtr(&_setup_ap_pc6).log(),
      //        size_t(&_setup_ap_end_pc6 - &_setup_ap_pc6));
      // asm volatile("wbinvd");

      // turn on caches during re-entry from CC6 sleep
      auto cc6 = reinterpret_cast<uint32_t volatile*>(MMIO_ADDR+SBOX_BASE+SBOX_C6_SCRATCH0);
      *cc6 |= 0x8000; // C1-CC6 MAS (bit 15)
    }

    void init_thread(size_t /*apicID*/)
    {
      //coreState.set(&coreStates[apicID/4]);
    }

    NORETURN void go_sleeping() SYMBOL("idle_sleep");

    void sleep()
    {
      size_t apicID = cpu::hwThreadID(); /// @todo should be getApicID()
      auto prev = coreStates[apicID/4].cc6ready.fetch_or(uint8_t(1 << (apicID%4)));
      if (prev | (1 << (apicID%4))) { // enable cc6
        /// @todo is this really needed if we always go into cc6?
        auto val = x86::getMSR(MSR_CC6_STATUS);
        val |= 0x1f;
        x86::setMSR(MSR_CC6_STATUS,val);
      }
      go_sleeping();
    }

    void wokeup(size_t /*apicID*/, size_t reason)
    {
      if (reason == 1) go_sleeping(); // woke up from CC6 => just sleep again
    }

    void wokeupFromInterrupt()
    {
      size_t apicID = cpu::hwThreadID(); /// @todo should be getApicID()
      auto prev = coreStates[apicID/4].cc6ready.fetch_and(uint8_t(~(1u << (apicID%4))));
      if (prev == 0xf) { // disable cc6
        auto val = x86::getMSR(MSR_CC6_STATUS);
        val &= ~0x1f;
        x86::setMSR(MSR_CC6_STATUS,val);
      }
    }

  } // namespace idle
} // namespace mythos
