#pragma once

namespace mythos {

class IOApicDef {
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

};

} // namepsace mythos
