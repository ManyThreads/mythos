#pragma once

#include "boot/memory-layout.h"

namespace mythos {
namespace sbox {
  constexpr uint64_t SBOX_THERMAL_STATUS           = 0x00001018;
  constexpr uint64_t SBOX_THERMAL_INTERRUPT_ENABLE = 0x0000101C;
  constexpr uint64_t SBOX_BOARD_TEMP1              = 0x00001030;
  constexpr uint64_t SBOX_BOARD_TEMP2              = 0x00001034;
  constexpr uint64_t SBOX_THERMAL_STATUS_INTERRUPT = 0x0000107C;

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

  static void sbox_write(uint64_t reg_offset, uint32_t value) {
    uint32_t* volatile addr = (uint32_t* volatile)(MMIO_ADDR + reg_offset);
    *addr = value;
  }

  static uint32_t sbox_read(uint64_t reg_offset) {
    uint32_t* volatile addr = (uint32_t* volatile)(MMIO_ADDR + reg_offset);
    return *addr;
  }

  static inline void enable_interrupts()
  {
    THERMAL_INTERRUPT_ENABLE_REG reg;
    reg.gpu_hot_int_enab = 1;
    reg.pwralert_int_enab = 1;
    reg.high_temp_int_enab = 1;
    sbox_write(SBOX_THERMAL_INTERRUPT_ENABLE, reg.value);
  }

} // namespace sbox
} // namespace mythos
