#pragma once

#include "boot/memory-layout.h"

namespace mythos {
  namespace sbox {

    constexpr uint64_t SBOX_THERMAL_STATUS           = 0x00001018;
    constexpr uint64_t SBOX_THERMAL_INTERRUPT_ENABLE = 0x0000101C;
    constexpr uint64_t SBOX_BOARD_TEMP1              = 0x00001030;
    constexpr uint64_t SBOX_BOARD_TEMP2              = 0x00001034;
    constexpr uint64_t SBOX_THERMAL_STATUS_INTERRUPT = 0x0000107C;
    constexpr uint64_t SBOX_GPU_HOT_CONFIG           = 0x00001068;
    constexpr uint64_t SBOX_COREVOLT                 = 0x00004104;
    constexpr uint64_t SBOX_DIE_TEMP0                = 0x0000103C;
    constexpr uint64_t SBOX_DIE_TEMP1                = 0x00001040;
    constexpr uint64_t SBOX_DIE_TEMP2                = 0x00001044;
    constexpr uint64_t SBOX_ICR0                     = 0x0000A9D0;
    constexpr uint64_t SBOX_MAX_DOORBELL             = 8;

    BITFIELD_DEF(uint32_t, THERMAL_INTERRUPT_ENABLE_REG)
      UIntField<value_t, base_t,0,1> high_temp_int_enab;
    UIntField<value_t, base_t,1,1> low_temp_int_enab;
    UIntField<value_t, base_t,2,1> out_of_spec_int_enab;
    UIntField<value_t, base_t,3,1> fan_monitor_int_enab;
    UIntField<value_t, base_t,4,1> system_monitor_int_enab;
    UIntField<value_t, base_t,5,1> mclk_ratio_int_enab;
    UIntField<value_t, base_t,6,1> alert_int_enab;
    UIntField<value_t, base_t,5,1> gpu_hot_int_enab;
    UIntField<value_t, base_t,8,1> pwralert_int_enab;
    UIntField<value_t, base_t,9,1> rsvd0;
    UIntField<value_t, base_t,10,10> sw_threshold1_temp;
    UIntField<value_t, base_t,20,1>  sw_threshold1_enab;
    UIntField<value_t, base_t,21,10> sw_threshold2_temp;
    UIntField<value_t, base_t,31,1>  sw_threshold2_enab;
    THERMAL_INTERRUPT_ENABLE_REG() : value(0) {}
    BITFIELD_END

      BITFIELD_DEF(uint32_t, GPU_HOT_CONFIG)
      UIntField<value_t, base_t,0,1> enab_freq_throttle;
    UIntField<value_t, base_t,1,1> gpuhot_enab; //enable mic assertion gpuhot
    UIntField<value_t, base_t,2,30> rsvd1;
    GPU_HOT_CONFIG() : value(0) {}
    BITFIELD_END

      BITFIELD_DEF(uint32_t, CORE_VOLTAGE_REG)
      UIntField<value_t, base_t,0,8> vid;
    UIntField<value_t, base_t,8,24> rsvd1;
    CORE_VOLTAGE_REG() : value(0) {}
    BITFIELD_END

      BITFIELD_DEF(uint32_t, DIE_TEMP_REG)
      UIntField<value_t, base_t,0,10>  sensor0;
    UIntField<value_t, base_t,10,10> sensor1;
    UIntField<value_t, base_t,20,10> sensor2;
    UIntField<value_t, base_t,30,2>  rsvd;
    DIE_TEMP_REG() : value(0) {}
    BITFIELD_END


    BITFIELD_DEF(uint32_t, THERMAL_STATUS_INT_REG)
    UIntField<value_t, base_t,0,1>  mclk_ratio_status;// bit 0 This bit is set whenever MCLK Ratio Changes. Cleared by SW writing.
    UIntField<value_t, base_t,1,1> mclk_ratio_log;// bit 1 This bit is also a sticky version of MCLK_Ratio_Status
    UIntField<value_t, base_t,2,1> alert_status;// bit 2 This bit is set whenever ALERT# pin is asserted. Cleared by SW writing.
    UIntField<value_t, base_t,3,1> alert_log;// bit 3 This bit is a sticky version of Alert_Status, cleared by s/w or by reset
    UIntField<value_t, base_t,4,1> gpuhot_status;// bit 4 This bit reflects the real-time value of the GPUHOT# pin (synchronized to SCLK domain).
    UIntField<value_t, base_t,5,1> gpuhot_log; // bit 5 This bit is set on the assertion edge of GPUHOT# and remains set until software clears it by doing a write.
    UIntField<value_t, base_t,6,1> poweralert_status;// bit 6 This bit reflects the real-time value of the PWRALERT# pin (synchronized to SCLK domain).
    UIntField<value_t, base_t,7,1> poweralert_log;// bit 7 This bit is set on the assertion edge of PWRALERT# and remains set until software clears it by doing a write.
    UIntField<value_t, base_t,8,23> rsvd0;
    UIntField<value_t, base_t,31,1> etc_freeze;// bit 31 This bit freeze the increment of elapsed-time counter

    THERMAL_STATUS_INT_REG() : value(0) {}
    BITFIELD_END

    static inline void sbox_write(uint64_t reg_offset, uint32_t value) {
      volatile uint32_t *addr = (volatile uint32_t*)(MMIO_ADDR + SBOX_BASE + reg_offset);
      *addr = value;
    }

    static inline uint32_t sbox_read(uint64_t reg_offset) {
      volatile uint32_t *addr = (volatile uint32_t*)(MMIO_ADDR + SBOX_BASE + reg_offset);
      return *addr;
    }

    static inline uint32_t sbox_vid() {
      CORE_VOLTAGE_REG creg;
      creg.value = sbox_read(SBOX_COREVOLT);
      return creg.vid;
    }

    static inline uint32_t sbox_die_temp(int sensor) {
      DIE_TEMP_REG temp;
      temp.value = sbox_read(SBOX_DIE_TEMP0);
      switch(sensor) {
        case 0:
          return temp.sensor0;
        case 1:
          return temp.sensor1;
        case 2:
          return temp.sensor2;
        default:
          return 0;
      }
    }

    static inline uint32_t thermal_status() {
      THERMAL_STATUS_INT_REG reg;
      reg.value = sbox_read(SBOX_THERMAL_STATUS_INTERRUPT);
      MLOG_ERROR(mlog::boot, reg.gpuhot_status, reg.gpuhot_log);
      return reg.value;
    }

    /**
     * Sends doorbell interrupt to itself (hopefully)
     * by setting icr bit 13
     * http://elixir.free-electrons.com/linux/v4.9/source/drivers/misc/mic/card/mic_x100.c#L76
     */
    static inline void send_interrupt(int doorbell) {
      uint64_t offset = SBOX_ICR0 + doorbell*8;
      uint32_t icr_low = sbox_read(offset);
      icr_low |=  (1 << 13); // send_icr bit 13
      //MLOG_ERROR(mlog::boot, DVARhex(offset), DVARhex(icr_low));
      sbox_write(offset, icr_low);
      icr_low = sbox_read(offset);
      //MLOG_ERROR(mlog::boot, DVARhex(icr_low));
    }

    static void enable_interrupts()
    {
      THERMAL_INTERRUPT_ENABLE_REG reg;
      reg.gpu_hot_int_enab = 1;
      reg.pwralert_int_enab = 1;
      reg.high_temp_int_enab = 1;
      reg.sw_threshold1_temp = 20;
      reg.sw_threshold1_enab = 1;
      reg.sw_threshold2_temp = 70;
      reg.sw_threshold2_enab = 1; // Triggers thermal interrupt at beginning for temp=70
      sbox_write(SBOX_THERMAL_INTERRUPT_ENABLE, reg.value);

      //GPU_HOT_CONFIG hreg;
      //hreg.gpuhot_enab = 1;
      //sbox_write(SBOX_GPU_HOT_CONFIG, reg.value);

      //MLOG_ERROR(mlog::boot, "Current Voltage",sbox_vid());
      //MLOG_ERROR(mlog::boot, "Current Temperatur:",sbox_die_temp(0));
    }

} // namespace sbox
} // namespace mythos
