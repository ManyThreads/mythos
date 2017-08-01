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

#pragma once

namespace mythos {

class IOApicDef {
public:
    static const uint64_t BASE_IRQ = 0x21;

	enum SelectionRegister {
		IOREGSEL = 0x00,
		IOWIN 	 = 0x10,
	};

    /**
     * Register indexes for IOAPIC
     */
	enum Register {
		IOAPICID	  = 0x00,
		IOAPICVER	  = 0x01,
		IOAPICARB	  = 0x02,
		IOREDTBL_BASE = 0x10,
	};

    // how destination of red table entry is interpreted
    enum Destmode {
        PHYS_MODE    = 0x0, //  Bits 56-59 contain an APIC ID
        LOGICAL_MODE = 0x1, // Bits 56-63 are interpreted as set of processors
    };

	BITFIELD_DEF(uint32_t, IOAPIC_ID)
    UIntField<value_t,base_t, 0,24> reserved1;
    UIntField<value_t,base_t, 24,4> id;
    UIntField<value_t,base_t, 28,4> reserved2;
    IOAPIC_ID() : value(0) {}
    BITFIELD_END

    BITFIELD_DEF(uint32_t, IOAPIC_VERSION)
    UIntField<value_t,base_t, 0,8> version;
    UIntField<value_t,base_t, 8,8> reserved1;
    UIntField<value_t,base_t, 16,8> max_redirection_table;
    UIntField<value_t,base_t, 24,8> reserved2s;
    IOAPIC_VERSION() : value(0) {}
    BITFIELD_END
public:
    virtual void init(size_t base_address_) = 0;
    virtual uint32_t read(size_t reg) = 0;
    virtual void write(size_t reg, uint32_t value) = 0;
    virtual void maskIRQ(uint64_t irq) = 0;
    virtual void unmaskIRQ(uint64_t irq) = 0;

private:
    virtual void setBase(size_t base_address_) = 0;
};

} // namepsace mythos
