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
#include "boot/DeployKernelSpace.hh"
#include "boot/pagetables.hh"
#include "mythos/HostInfoTable.hh"
#include "cpu/clflush.hh"

namespace mythos {
  namespace boot {

    extern volatile uint64_t hostInfoPtr SYMBOL("_host_info_ptr");
    HostInfoTable host_info;
    HostInfoTable::DebugChannel debugOut;
    HostInfoTable::CtrlChannel ctrlOut;
    HostInfoTable::CtrlChannel ctrlIn;

    void initKernelSpace() {
      // map XeonPhi MMIO area
      static_assert(MMIO_ADDR == 0xffff800100600000, "failed assumption about kernel layout");
      devices_pml2[3] = CD + PRESENT + WRITE + ACCESSED + DIRTY + ISPAGE + GLOBAL + MMIO_PHYS;

      // initialise the communication channels
      debugOut.init();
      ctrlOut.init();
      ctrlIn.init();
      host_info.debugOut = physPtrFromImage(&debugOut).physint();
      host_info.ctrlOut = physPtrFromImage(&ctrlOut).physint();
      host_info.ctrlIn = physPtrFromImage(&ctrlIn).physint();
      cpu::clflush(&host_info);
      hostInfoPtr = physPtrFromImage(&host_info).physint(); // this notifies the host

      initKernelSpaceCommon(); // loads the new page table, which makes the init segment inaccessible!

      // tell the host's loader kernel module, that booting is finished
      auto scratch2 = reinterpret_cast<uint32_t*>(MMIO_ADDR+SBOX_BASE+SBOX_SCRATCH2);
      *scratch2 |= 1;
    }

  } // boot
} // namespace mythos
