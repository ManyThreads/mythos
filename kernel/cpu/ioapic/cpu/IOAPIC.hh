#pragma once

#include "util/bitfield.hh"

namespace mythos {

class IOAPIC {
public:
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

    BITFIELD_DEF(uint64_t, RED_TABLE_ENTRY)
    UIntField<value_t,base_t, 0,8> intvec; // interrupt vector from 0x20 to 0xFE
    UIntField<value_t,base_t, 8,3> delmode; // 0=normal,1=lowpriority,2=system_management,4=NMI,5=INIT,7=external
    UIntField<value_t,base_t, 11,1> destmode; //0=physical, 1=logical
    UIntField<value_t,base_t, 12,1> delivs; // Set if this interrupt is going to be sent, but the APIC is busy
    UIntField<value_t,base_t, 13,1> intpol; //polarity: 0=high active, 1 = low active
    UIntField<value_t,base_t, 14,1> remote_irr; // response from lapic or EOI
    UIntField<value_t,base_t, 15,1> trigger_mode; //0 edge sensitive, 1 level sensitive
    UIntField<value_t,base_t, 16,1> int_mask; //interrupt masked if 1 (to disable)
    UIntField<value_t,base_t, 17,39> reserved1;
    UIntField<value_t,base_t, 56,8> dest; // dependend on destmode: 0: apicid; 1:set of processors
    UIntField<value_t,base_t, 0, 32> lower;
    UIntField<value_t,base_t, 32, 32> upper;
    RED_TABLE_ENTRY() : value(0) {}
    BITFIELD_END

    IOAPIC(char *base_address_)
        :base_address(base_address_) 
    {}

    IOAPIC(size_t base_address_)
        :base_address((char*)base_address_) 
    {}

    uint32_t read(size_t reg) {
        uint32_t volatile *ioapic = (uint32_t volatile *)base_address;
        ioapic[0] = (reg & 0xff);
        MLOG_ERROR(mlog::boot, "IOAPIC: read", DVARhex(ioapic[4]), DVARhex(reg));
        return ioapic[4];
    }

    void write(size_t reg, uint32_t value) {
        MLOG_ERROR(mlog::boot, "IOAPIC: write", DVARhex(base_address), DVARhex(reg), DVARhex(value));
        uint32_t volatile *ioapic = (uint32_t volatile *)base_address;
        ioapic[0] = (reg & 0xff);
        ioapic[4] = value;
    }

private:
    char *base_address = {nullptr};
};

} // namespace mythos