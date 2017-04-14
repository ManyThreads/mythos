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
#pragma once

#include "util/compiler.hh"
#include <cstdint>

namespace mythos {

  /** Interrupt Gate Descriptor for the IDT.
   * @author Maik Krueger
   */
  class PACKED IRQDescriptorAmd64
  {
  public:
    /* The processor checks the DPL of the interrupt or trap gate only
     * if an exception or interrupt is generated with an INT n, INT 3,
     * or INTO instruction. Here, the CPL must be less than or equal
     * to the DPL of the gate. This restriction prevents application
     * programs or procedures running at privilege level 3 from using
     * a software interrupt to access critical exception handlers,
     * such as the page-fault handler, providing that those handlers
     * are placed in more privileged code segments (numerically lower
     * privilege level). For hardware-generated interrupts and
     * processor-detected exceptions, the processor ignores the DPL of
     * interrupt and trap gates.
    */
    enum PrivilegeLevel {
      SYSTEM = 0x0,
      USER   = 0x3
    };
    enum Type {
      TASK_GATE      = 0x05,
      INTERRUPT_GATE = 0x0E,
      TRAP_GATE      = 0x0F
    };

    IRQDescriptorAmd64() : present(0) {}

    void setKernelIRQ(uint64_t offset, uint16_t selector, uint8_t ist) {
      set(SYSTEM, INTERRUPT_GATE, offset, selector, ist);
    }
    
    void set(PrivilegeLevel priv, Type type, uint64_t offset, uint16_t selector, uint8_t ist) {
      this->offset0 = uint16_t(offset);
      this->offset16 = uint16_t(offset>>16);
      this->offset32 = uint32_t(offset>>32);
      this->csseg = selector;
      this->ist = ist & 7;
      this->zeros = 0;
      this->type = type & 0xF;
      this->dpl = priv & 3;
      this->present = 1;
    }

    void setIST(uint8_t ist) { this->ist = ist & 7; }

    /** @brief Return the address of the interrupt handler function */
    uint64_t getOffset() const {
      return uint64_t(offset32) << 32 | uint64_t(offset16)<<16 | uint64_t(offset0);
    }

    uint16_t getSegmentSelector() const { return csseg; }
    Type getType() const { return Type(type); }

  private:
    uint32_t offset0:16, csseg:16;
    uint32_t ist:3, zeros:5, type:5, dpl:2, present:1, offset16:16;
    uint32_t offset32;
    uint32_t :32;
  };

} // namespace mythos
