#pragma once

#include "util/bitfield.hh"

namespace mythos {

class IOAPIC {
	enum memoryMapped {
		IOREGSEL = 0x00,
		IOWIN 	 = 0x10,
	};

	enum internalRegs {
		IOAPICID	  = 0x00,
		IOAPICVER	  = 0x01,
		IOAPICARB	  = 0x02,
		IOREDTBL_BASE = 0x10,
	};

    // how destination of red table entry is interpreted
    enum destmode { 
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
    UIntField<value_t,base_t, 0,8> intvec;
    UIntField<value_t,base_t, 8,3> delmode;
    UIntField<value_t,base_t, 11,1> destmode;
    UIntField<value_t,base_t, 12,1> delivs;
    UIntField<value_t,base_t, 13,1> intpol;
    UIntField<value_t,base_t, 14,1> remote_irr;
    UIntField<value_t,base_t, 15,1> trigger_mode; //0 edge sensitive, 1 level sensitive
    UIntField<value_t,base_t, 16,1> int_mask; //interrupt masked if 1
    UIntField<value_t,base_t, 17,39> trigger_mode;
    UIntField<value_t,base_t, 56,8> dest; // dependend on destmode: 0: apicid; 1:set of processors
    RED_TABLE_ENTRY() : value(0) {}
    BITFIELD_END
};

} // namespace mythos