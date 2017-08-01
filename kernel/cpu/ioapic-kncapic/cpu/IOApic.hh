#pragma once

#include "util/bitfield.hh"
/* -*- mode:C++; indent-tabs-mode:nil; -*- */
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
 * Copyright 2017 Randolf Rotta, Robert Kuban, and contributors, BTU Cottbus-Senftenberg
 */

#include "cpu/IOApicDef.hh"

namespace mythos {

class IOApic : public IOApicDef {
public:

    // An IO Apic Route entry
    BITFIELD_DEF(uint64_t, RED_TABLE_ENTRY)
    UIntField<value_t,base_t, 0,8> intvec; // interrupt vector from 0x20 to 0xFE (typical)
    UIntField<value_t,base_t, 8,3> delmode; // 0=normal,1=lowpriority,2=system_management,4=NMI,5=INIT,7=external
    UIntField<value_t,base_t, 11,1> destmode; //0=physical, 1=logical
    UIntField<value_t,base_t, 12,1> delivs; // Set if this interrupt is going to be sent, but the APIC is busy
    UIntField<value_t,base_t, 13,1> intpol; //polarity: 0=high active, 1 = low active
    UIntField<value_t,base_t, 14,1> remote_irr; // response from lapic or EOI
    UIntField<value_t,base_t, 15,1> trigger_mode; //0 edge sensitive, 1 level sensitive
    UIntField<value_t,base_t, 16,1> int_mask; //interrupt masked if 1 (to disable)
    UIntField<value_t,base_t, 17,15> reserved1;
    UIntField<value_t,base_t, 32,8> dest; // dependend on destmode: 0: apicid; 1:set of processors
    UIntField<value_t,base_t, 40,24> reserved2;
    UIntField<value_t,base_t, 0, 32> lower;
    UIntField<value_t,base_t, 32, 32> upper;
    RED_TABLE_ENTRY() : value(0) {}
    BITFIELD_END

    IOApic ()
        :base_address(0)
    {}

    IOApic(uint32_t volatile *base_address_)
        :base_address(base_address_)
    { init(); }

    IOApic(size_t base_address_)
        :base_address((uint32_t volatile *) base_address_)
    { init(); }

    void init(size_t base_address_) override { setBase(base_address_); init(); }
    uint32_t read(size_t reg) override;
    void write(size_t reg, uint32_t value) override;
    void maskIRQ(uint64_t irq) override;
    void unmaskIRQ(uint64_t irq) override;
    RED_TABLE_ENTRY readTableEntry(size_t table_entry);
    void writeTableEntry(size_t table_entry, RED_TABLE_ENTRY rte);
private:
    void init();
    void setBase(size_t base_address_) override { base_address = (uint32_t volatile*) base_address_; }
    uint32_t volatile *base_address = {nullptr};
};

extern IOApic ioapic;

} // namespace mythos
