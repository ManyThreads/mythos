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
    MLOG_ERROR(mlog::boot, DVAR(creg.vid));
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

  static void enable_interrupts()
  {
    THERMAL_INTERRUPT_ENABLE_REG reg;
    reg.gpu_hot_int_enab = 1;
    reg.pwralert_int_enab = 1;
    reg.high_temp_int_enab = 1;
    reg.sw_threshold1_temp = 44;
    reg.sw_threshold1_enab = 1;
    reg.sw_threshold2_temp = 44;
    reg.sw_threshold2_enab = 1;
    sbox_write(SBOX_THERMAL_INTERRUPT_ENABLE, reg.value);

    GPU_HOT_CONFIG hreg;
    hreg.gpuhot_enab = 1;
    sbox_write(SBOX_GPU_HOT_CONFIG, reg.value);

    MLOG_ERROR(mlog::boot, "Current Voltage",sbox_vid());
    MLOG_ERROR(mlog::boot, "Current Temperatur:",sbox_die_temp(0));
  }

} // namespace sbox
} // namespace mythos
