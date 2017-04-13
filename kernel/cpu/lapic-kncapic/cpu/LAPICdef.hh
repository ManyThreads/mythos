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
 * Copyright 2016 Randolf Rotta, and contributors, BTU Cottbus-Senftenberg
 */
#pragma once

#include <cstddef> // for size_t
#include <cstdint> // for uint32_t etc
#include "util/bitfield.hh"

namespace mythos {

  class LAPICdef
  {
  public:
    enum RegisterAddr {
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

    BITFIELD_DEF(uint32_t, Register)
    UIntField<value_t,base_t, 23,9> apic_id; // REG_APICID
    UIntField<value_t,base_t, 0,8> version; // REG_VERSION
    UIntField<value_t,base_t, 16,8> max_lvt_entry; // REG_VERSION
    BoolField<value_t,base_t, 24> eio_sup_supported; // REG_VERSION EIO broadcast suppression supported
    UIntField<value_t,base_t, 0,8> vector; // REG_ICR_LOW and REG_LVT and REG_SVR
    UIntField<value_t,base_t, 8,3> delivery_mode; // REG_ICR_LOW and REG_LVT
    BoolField<value_t,base_t, 11> logical_destination; // REG_ICR_LOW
    BoolField<value_t,base_t, 12> delivery_pending; // REG_ICR_LOW and REG_LVT
    BoolField<value_t,base_t, 14> level; // REG_ICR_LOW
    BoolField<value_t,base_t, 15> level_triggered; // REG_ICR_LOW
    UIntField<value_t,base_t, 18,2> destination_shorthand; // REG_ICR_LOW
    UIntField<value_t,base_t, 16,16> destination; // REG_ICR_HIGH and REG_LDR
    UIntField<value_t,base_t, 28,4> model; // REG_DFR flat vs cluster
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
  };

} // namespace mythos
