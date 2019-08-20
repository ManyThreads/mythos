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

#include "boot/memory-layout.h"
#include "cpu/hwthreadid.hh"
#include "cpu/LAPICdef.hh"

namespace mythos {
  class LAPIC;
  extern LAPIC lapic;

  class LAPIC
    : public LAPICdef
  {
  public:
    void init();
    bool isEnabled() { return read(REG_SVR).apic_enable; }
    uint32_t getId() { return read(REG_APICID).apic_id;  }
    void setId(uint32_t id) { write(REG_APICID, read(REG_APICID).apic_id(id)); }
    uint8_t getVersion() { return uint8_t(read(REG_VERSION).version); }
    uint8_t getMaxLVT() { return uint8_t(read(REG_VERSION).max_lvt_entry); }

    // TODO change interface to pass divider (3bit) and timer mode, may leave convenience methods
    void enablePeriodicTimer(uint8_t irq, uint32_t count);
    void disableTimer();

    /** initial count register contains the start value, current count
     * register is decremented until reaching zero. writing the
     * initial count will reset the current count.
     */
    void setInitialCount(uint32_t count) { write(REG_TIMER_ICR, count); }
    uint32_t getCurrentCount() { return read(REG_TIMER_CCR).value; }

    bool broadcastInitIPIEdge();
    bool sendInitIPIEdge(cpu::ApicID apicid);
    bool broadcastStartupIPI(size_t startIP);
    bool sendStartupIPI(cpu::ApicID apicid, size_t startIP);
    bool sendNMI(size_t destination);
    bool sendIRQ(size_t destination, uint8_t vector);
    void endOfInterrupt() { write(REG_EOI, 0); }

  protected:
    static Register edgeIPI(IrcDestinationShorthand dest, IcrDeliveryMode mode, uint8_t vec) {
      return Register().destination_shorthand(dest).level_triggered(0).level(1)
        .logical_destination(0).delivery_mode(mode).vector(vec)
        .delivery_pending(0); //workaround for qemu
    }

    static Register read(size_t reg) {
      return *((volatile uint32_t*)(LAPIC_ADDR + reg));
    }

    static void write(size_t reg, Register value) {
      *(volatile uint32_t*)(LAPIC_ADDR + reg) = value.value;
    }

    void writeIPI(size_t destination, Register icrlow);
  };

} // namespace mythos
