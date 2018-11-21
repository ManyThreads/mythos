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
 * Copyright 2014 Randolf Rotta, Maik Krüger, and contributors, BTU Cottbus-Senftenberg
 */

#include "boot/DeployHWThread.hh"
#include "util/PhysPtr.hh"
#include "util/mstring.hh" // for memcpy

extern char _setup_ap;
extern char _setup_ap_end;

namespace mythos {
  namespace boot {

    ALIGN_4K char DeployHWThread::stackspace[CORE_STACK_SIZE*MYTHOS_MAX_THREADS];
    ALIGN_4K char DeployHWThread::nmistackspace[NMI_STACK_SIZE*MYTHOS_MAX_THREADS];
    uintptr_t DeployHWThread::stacks[MYTHOS_MAX_APICID];
    IdtAmd64 DeployHWThread::idt;

    SchedulingContext schedulers[MYTHOS_MAX_THREADS];
    CoreLocal<SchedulingContext*> localScheduler KERNEL_CLM;

    InterruptControl interruptController[MYTHOS_MAX_THREADS];
    CoreLocal<InterruptControl*> localInterruptController KERNEL_CLM;

    void initAPTrampoline(size_t startIP) {
      PhysPtr<uint16_t> trampoline_phys_high(0x469ul);
      PhysPtr<uint16_t> trampoline_phys_low(0x467ul);
      *trampoline_phys_high = uint16_t(startIP >> 4);
      *trampoline_phys_low = uint16_t(startIP & 0xf);

      ASSERT((startIP & 0x0FFF) == 0 && ((startIP>>12)>>8) == 0);
      memcpy(PhysPtr<char>(startIP).log(),
	     physPtr(&_setup_ap).log(),
	     size_t(&_setup_ap_end - &_setup_ap));
      asm volatile("wbinvd");
    }

  } // namespace boot
} // namespace mythos
