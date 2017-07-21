#pragma once

#include "util/bitfield.hh"
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

    IOApic(uint32_t volatile *base_address_)
        :base_address(base_address_)
    { init(); }

    IOApic(size_t base_address_)
        :base_address((uint32_t volatile *)base_address_)
    { init(); }

    void setBase(size_t base_address_) { base_address = (uint32_t volatile*) base_address_; }

    uint32_t read(size_t reg);

    void write(size_t reg, uint32_t value);

private:
    void init();
    uint32_t volatile *base_address = {nullptr};
};

} // namespace mythos
