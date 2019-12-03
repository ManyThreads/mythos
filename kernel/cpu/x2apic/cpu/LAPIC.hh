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
 * Copyright 2019 Randolf Rotta and contributors, BTU Cottbus-Senftenberg
 */
#pragma once

#include "util/bitfield.hh"

namespace mythos {
//namespace cpu {

// TODO should be moved to plattform-specific module ?
class X2Apic;
extern X2Apic lapic;

class X2Apic
{
public: // public methods for mythos
    void init();
    uint32_t getId();
    //uint8_t getVersion() { return uint8_t(read(ApicConfig::REG_VERSION).version); }
    //uint8_t getMaxLVT() { return uint8_t(read(ApicConfig::REG_VERSION).max_lvt_entry); }

    void startupBroadcast(size_t startIP);
    void startup(uint32_t apicid, size_t startIP);
    void sendNMI(uint32_t apicid);
    void sendIRQ(uint32_t apicid, uint8_t vector);
    void endOfInterrupt();

public: // plattform specific constants and types

    enum RegisterAddr {
      BASE              = 0x800,
      REG_APICID        = BASE+0x2, // Local ID (RO)
      REG_VERSION       = BASE+0x3,
      REG_TPR           = BASE+0x8, // Task Priority Register
      REG_PPR           = BASE+0xA, // Processor Priority Register
      REG_EOI           = BASE+0xB, // End of Interrupt
      REG_LDR           = BASE+0xD, // Logical Destination Register aka logical APIC ID (RO)
      REG_SVR           = BASE+0xF, // Spurious Vector Register (RW)
      REG_ISR           = BASE+0x10, // In-Service Register
      REG_TMR           = BASE+0x18, // Trigger Mode Register
      REG_IRR           = BASE+0x20, // Interrupt Request Register
      REG_ESR           = BASE+0x28, // Error Status Register
      REG_ICR           = BASE+0x30, // Interrupt Command Register (W)
      REG_LVT_TIMER     = BASE+0x32, // Local Vector Table: Timer
      REG_LVT_THERMAL   = BASE+0x33, // Local Vector Table: Thermal
      REG_LVT_PERFCNT   = BASE+0x34, // Local Vector Table: Performance Monitoring Counter
      REG_LVT_LINT0     = BASE+0x35, // Local Vector Table: LINT0
      REG_LVT_LINT1     = BASE+0x36, // Local Vector Table: LINT1
      REG_LVT_ERROR     = BASE+0x37, // Local Vector Table: Error Register
      REG_TIMER_ICR     = BASE+0x38, // Initial Counter Register (RW)
      REG_TIMER_CCR     = BASE+0x39, // Current Counter Register (RO)
      REG_TIMER_DCR     = BASE+0x3E, // Divide Configuration Register (RW)
      REG_SELF_IPI      = BASE+0x3F, // Self-IPI (W)
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

    BITFIELD_DEF(uint64_t, RegLDR)
    UIntField<value_t,base_t, 0,16> logicalID;
    UIntField<value_t,base_t, 16,16> clusterID;
    RegLDR() : value(0) {}
    BITFIELD_END

    BITFIELD_DEF(uint64_t, RegICR)
    UIntField<value_t,base_t, 0,8> vector;
    UIntField<value_t,base_t, 32,32> destination;
    UIntField<value_t,base_t, 8,3> delivery_mode; // TODO should have enum field...
    BoolField<value_t,base_t, 11> logical_destination;
    BoolField<value_t,base_t, 14> level;
    BoolField<value_t,base_t, 15> level_triggered;
    UIntField<value_t,base_t, 18,2> destination_shorthand;
    RegICR() : value(0) {}
    BITFIELD_END


    BITFIELD_DEF(uint64_t, Register)
    BoolField<value_t,base_t, 24> eio_sup_supported; // REG_VERSION EIO broadcast suppression supported
    UIntField<value_t,base_t, 0,4> task_prio_sub; // REG_TPR
    UIntField<value_t,base_t, 4,4> task_prio; // REG_TPR
    BoolField<value_t,base_t, 8> apic_enable; // REG_SVR
    BoolField<value_t,base_t, 9> focus_processor_checking; // REG_SVR
    BoolField<value_t,base_t, 12> eio_suppression; // REG_SVR
    BoolField<value_t,base_t, 13> pin_polarity; // REG_LVT
    BoolField<value_t,base_t, 14> remote_irr; // REG_LVT
    BoolField<value_t,base_t, 15> trigger_mode; // REG_LVT
    BoolField<value_t,base_t, 16> masked; // REG_LVT
    UIntField<value_t,base_t, 17,2> timer_mode; // REG_LVT
    Register() : value(0) {}
    BITFIELD_END

    enum {
      TIMER_DIVIDER  = 16ul,
      FREQUENCY_DIV  = 200ul
    };

    constexpr static uint8_t ENABLE_APIC = 8;
    constexpr static uint8_t APIC_BSP_FLAG = 8; //  Intel SDM, Vol. 3A, section 10.4.4.
};

//} // namespace cpu
} // namespace mythos
