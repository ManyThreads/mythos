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
#pragma once

#include "cpu/ctrlregs.hh"

namespace mythos {
  namespace cpu {

    /** FXSAVE structure to save and restore the FPU state.
     *
     * see http://www.sandpile.org/x86/fp_new.htm
     * and http://x86.renejeschke.de/html/file_module_x86_id_128.html
     */
    class FpuState
    {
    public:
      void clear() { 
          memset(this, 0, sizeof(FpuState));
          //save();
      }
      
      void save() { asm volatile("fxsaveq %0" : "=m" (*this)); }
      void restore() { asm volatile("fxrstorq %0" : "=m" (*this)); }

      static void initCpu() {
        x86::setCR0((x86::getCR0() & ~0xC) | 0x19);
        asm volatile ("clts");
        asm volatile ("fninit");
        /// @todo what about xcr0 ?
      }

    private:
      char raw[512] alignas(16);
    };

  } // namespace cpu
} // namespace mythos
