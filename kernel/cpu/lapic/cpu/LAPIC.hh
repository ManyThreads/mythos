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

#include "cpu/LAPICdef.hh"

namespace mythos {
  class XApic;
  extern XApic lapic;

  class XApic
    : public LAPICdef
  {
  public:
    XApic(uintptr_t base)
      : lapic_base(reinterpret_cast<volatile uint32_t*>(base)) {}

    void init();
    bool isEnabled() { return read(REG_SVR).apic_enable; }
    uint32_t getId() { return read(REG_APICID).apic_id;  }
    void setId(uint32_t id) { write(REG_APICID, read(REG_APICID).apic_id(id)); }
    uint8_t getVersion() { return uint8_t(read(REG_VERSION).version); }
    uint8_t getMaxLVT() { return uint8_t(read(REG_VERSION).max_lvt_entry); }

    void startupBroadcast(size_t startIP);
    void startup(uint32_t apicid, size_t startIP);
    void sendNMI(uint32_t apicid);
    void sendIRQ(uint32_t apicid, uint8_t vector);
    void endOfInterrupt() { write(REG_EOI, 0); }

    /** initial count register contains the start value, current count
     * register is decremented until reaching zero. writing the
     * initial count will reset the current count.
     *
     * @todo change interface to pass divider (3bit)
     */
    void setTimerCounter(uint32_t count);
    uint32_t getTimerCounter();
    void enableTimer(uint8_t irq, bool periodic);
    void disableTimer();

  protected:
    Register edgeIPI(IrcDestinationShorthand dest, IcrDeliveryMode mode, uint8_t vec);
    Register read(size_t reg) { return lapic_base[reg/4]; }
    void write(size_t reg, Register value) { lapic_base[reg/4] = value.value; }
    void writeIPI(size_t destination, Register icrlow);
    void waitForIPI();

  protected:
    volatile uint32_t* lapic_base;
  };

} // namespace mythos
