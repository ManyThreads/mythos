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
#include <cstddef> // for size_t
#include <cstdint> // for uint32_t etc

namespace mythos {
  class LAPIC;
  extern LAPIC lapic;

  class LAPIC
  {
  public:
    void init();
    bool isEnabled() { return read(REG_SVR).svr.apic_enable; }
    uint32_t getId() { return read(REG_APICID).id.apic_id;  }
    void setId(uint32_t id) {
      auto value = read(REG_APICID);
      value.id.apic_id = id & 0xFF;
      write(REG_APICID, value);
    }
    uint8_t getVersion() { return read(REG_VERSION).ver.version; }
    uint8_t getMaxLVT() { return read(REG_VERSION).ver.max_lvt_entry; }

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
    bool broadcastStartupIPI(size_t startIP);
    bool sendNMI(size_t destination);
    bool sendIRQ(size_t destination, uint8_t vector);
    void endOfInterrupt() { write(REG_EOI, 0); }

  protected:
    enum Register {
      REG_APICID                = 0x20,
      REG_VERSION               = 0x30,
      REG_LDR                   = 0xD0,
      REG_DFR                   = 0xE0,
      REG_SVR                   = 0xF0,
      REG_ISR                   = 0x100,
      REG_IRR                   = 0x200,
      REG_ESR                   = 0x280,
      REG_LVT_CMCI              = 0x2F0,
      REG_ICR_LOW               = 0x300,
      REG_ICR_HIGH              = 0x310,
      REG_LVT_TIMER             = 0x320,
      REG_LVT_THERMAL           = 0x330,
      REG_LVT_PERFCNT           = 0x340,
      REG_LVT_LINT0             = 0x350,
      REG_LVT_LINT1             = 0x360,
      REG_LVT_ERROR             = 0x370,
      REG_TIMER_ICR             = 0x380,
      REG_TIMER_CCR             = 0x390,
      REG_TIMER_DCR             = 0x3E0,
      REG_TASK_PRIO             = 0x80,
      REG_EOI                   = 0xB0
    };

    enum IcrDeliveryMode {
      MODE_FIXED            = 0x0,
      MODE_LOWPRIO          = 0x1,
      MODE_SMI              = 0x2,
      MODE_NMI              = 0x4,
      MODE_INIT             = 0x5,
      MODE_SIPI             = 0x6,
      MODE_EXTINT           = 0x7
    };

    enum TimerMode {
        ONESHOT               = 0x0,
        PERIODIC              = 0x1,
        TSCDEADLINE           = 0x2
    };

    enum IrcDestinationShorthand {
      ICR_DESTSHORT_NO      = 0x0,
      ICR_DESTSHORT_SELF    = 0x1,
      ICR_DESTSHORT_ALL     = 0x2,
      ICR_DESTSHORT_NOTSELF = 0x3
    };

    enum EsrStatus {
      ESR_SEND_CS  = 0x00001,
      ESR_RECV_CS  = 0x00002,
      ESR_SEND_ACC = 0x00004,
      ESR_RECV_ACC = 0x00008,
      ESR_SENDILL  = 0x00020,
      ESR_RECVILL  = 0x00040,
      ESR_ILLREGA  = 0x00080
    };

    struct PACKED LAPICID {
      unsigned int :24, apic_id:8; // APIC ID
    };

    struct PACKED LAPICVER {
      unsigned int version:8, :8, // Version (0x14 for P4s and Xeons)
	max_lvt_entry:8, // Maximum LVT Entry
	eio_suppression:1, :7; // EIO broadcast suppression supported
    };
    
    struct PACKED ICR_LOW {
      unsigned int vector:8, // Vector
	delivery_mode:3, // Delivery Mode
	logical_destination:1, // Destination Mode
	delivery_pending:1, :1, // Delivery Status
	level:1, // Level
	level_triggered:1, :2, // Trigger Mode
	destination_shorthand:2, :12; // Destination Shorthand
    };

    struct PACKED ICR_HIGH {
      unsigned int :24, destination_field:8; // Destination Field
    };

    struct PACKED LDR {
      unsigned int :24, lapic_id:8; // Logical APIC ID
    };

    struct PACKED DFR {
      unsigned int :28,	model:4; // Model (Flat vs. Cluster)
    };

    struct PACKED TPR {
      unsigned int task_prio_sub:4, // Task Priority Sub-Class
	task_prio:4, :24; // Task Priority
    };

    struct PACKED SVR {
      unsigned int spurious_vector:8, // Spurious Vector
	apic_enable:1, // APIC Software Enable/Disable
	focus_processor_checking:1, :2, // Focus Processor Checking
	eio_suppression:1, :19; // EIO broadcast suppression enabled
    };

    struct PACKED LVT {
      unsigned int vector:8, // the destination interrupt handler
	delivery_mode:3, :1,
	delivery_status:1,
	pin_polarity:1,
	remote_irr:1,
	trigger_mode:1,
	masked:1,
	timer_mode:2, :13;
    };
    
    union PACKED LAPICRegister {
      LAPICID id;
      LAPICVER ver;
      ICR_LOW icr_l;
      ICR_HIGH icr_h;
      LDR ldr;
      DFR dfr;
      TPR tpr;
      SVR svr;
      LVT lvt;
      uint32_t value;

      LAPICRegister() : value(0) {}
      LAPICRegister(uint32_t value) : value(value) {}

      static LAPICRegister edgeIPI(IrcDestinationShorthand dest, IcrDeliveryMode mode, uint8_t vec) {
	LAPICRegister reg;
	reg.icr_l.destination_shorthand = dest & 3;
	reg.icr_l.level_triggered = 0;
	reg.icr_l.level = 1;
	reg.icr_l.logical_destination = 0;
	reg.icr_l.delivery_mode = mode & 7;
	reg.icr_l.vector = vec;
	reg.icr_l.delivery_pending = 0; //workaround for qemu
	return reg;
      }
    };
    
    enum {
      TIMER_DIVIDER  = 16ul,
      FREQUENCY_DIV  = 200ul
    };

  protected:
    LAPICRegister read(size_t reg);
    void write(size_t reg, LAPICRegister value);
    void writeIPI(size_t destination, LAPICRegister icrlow);
  };

} // namespace mythos
