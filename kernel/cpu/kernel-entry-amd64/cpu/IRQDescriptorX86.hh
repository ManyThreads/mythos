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

#include "util/compiler.hh"
#include <cstdint>

namespace mythos {

  /** Interrupt Gate Descriptor for the IDT.
   * @author Maik Krueger
   */
  class PACKED IRQDescriptorX86
  {
  public:
    enum PrivilegeLevel {
      SYSTEM = 0x0,
      USER   = 0x3
    };
    enum Type {
      TASK_GATE      = 0x05,
      INTERRUPT_GATE = 0x0E,
      TRAP_GATE      = 0x0F
    };

    IRQDescriptorX86() : present(0) {}

    void set(PrivilegeLevel priv, Type type, uint32_t offset, uint16_t selector) {
      this->offset16 = offset>>16;
      this->present = 1;
      this->dpl = priv;
      this->type = type;
      this->zeros = 0;
      this->csseg = selector;
      this->offset0 = offset & 0xFFFF;
    }

    /** address of the interrupt handler function */
    uint32_t getOffset() const { return offset16<<16 | offset0; }
    uint16_t getSegmentSelector() const { return csseg; }
    Type getType() const { return Type(type); }

  private:
    uint32_t offset0:16, csseg:16;
    uint32_t reserved:5, zeros:3, type:5, dpl:2, present:1, offset16:16;
  };

} // namespace mythos
