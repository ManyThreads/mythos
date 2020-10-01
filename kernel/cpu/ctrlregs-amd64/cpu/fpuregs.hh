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
 * Copyright 2019 Randolf Rotta and contributors, BTU Cottbus-Senftenberg
 */
#pragma once

#include "util/compiler.hh"
#include "util/bitfield.hh"
#include "cpu/ctrlregs.hh"
#include <cstdint>

namespace mythos {
    namespace x86 {

/** legacy x87 FPU state format for FSAVE and FRSTOR. */
struct alignas(16) PACKED FRegs
{
  uint32_t cwd;	// FPU Control Word
  uint32_t swd;	// FPU Status Word
  uint32_t twd;	// FPU Tag Word
  uint32_t fip;	// FPU IP Offset
  uint32_t fcs;	// FPU IP Selector
  uint32_t foo;	// FPU Operand Pointer Offset
  uint32_t fos;	// FPU Operand Pointer Selector
  uint32_t st_space[20]; //8*10 bytes for each FP-reg = 80 bytes
  uint32_t status; // Software status information, not touched by FSAVE
};

/** legacy x87 FPU state format for FXSAVE and FXRESTORE. */
struct alignas(16) PACKED FXRegs 
{
  uint16_t cwd; // Control Word
  uint16_t swd; // Status Word
  uint16_t twd; // Tag Word
  uint16_t fop; // Last Instruction Opcode
  union {
    struct {
      uint64_t rip; // Instruction Pointer
      uint64_t rdp; // Data Pointer
    } ipdp;
    struct {
      uint32_t fip; // FPU IP Offset
      uint32_t fcs; // FPU IP Selector
      uint32_t foo; // FPU Operand Offset
      uint32_t fos; // FPU Operand Selector
    } ipop;
  };
  uint32_t mxcsr; // MXCSR Register State
  uint32_t mxcsr_mask; // MXCSR Mask

  uint32_t st_space[32]; // 8*16 bytes for each FP-reg = 128 bytes
  uint32_t xmm_space[64]; // 16*16 bytes for each XMM-reg = 256 bytes
  uint32_t padding[12];

  union {
    uint32_t padding1[12];
    uint32_t sw_reserved[12];
  };
};

struct PACKED XStateHeader
{
  uint64_t xfeatures;
  uint64_t xcomp_bv;
  uint64_t reserved[6];
};

/** modern FPU state for XSAVE.
 * The actual size depends on the processor and enabled features. 
 */
struct alignas(64) PACKED XRegs
{
  FXRegs i387;
  XStateHeader header;
  uint8_t extended_state_area[8]; // because 0 is not allowed in ISO C++
};

/** union of all FPU state variants.
 * The padding is choosen to give enough room for the XSAVE extended state area. 
 */
union FpuState 
{
  FRegs fsave;
  FXRegs fxsave;
  XRegs xsave;
  uint8_t __padding[4096];
};


inline bool hasFPU() { return bits(cpuid(1,0).edx,0); }
inline bool hasMMX() { return bits(cpuid(1,0).edx,23); }
inline bool hasAVX() { return bits(cpuid(1,0).ecx,28); }
inline bool hasMPX() { return bits(cpuid(7,0).ebx,14); }
inline bool hasAVX512F() { return bits(cpuid(7,0).ebx,16); }
inline bool hasINTELPT() { return bits(cpuid(7,0).ebx,25); }
inline bool hasPKU() { return bits(cpuid(7,0).ecx,3); }
inline bool hasFXSR() { return bits(cpuid(1,0).edx,24); }
inline bool hasXMMEXCPT() { return bits(cpuid(1,0).edx,25); }
inline bool hasXSAVE() { return bits(cpuid(1,0).ecx,26); }
inline bool hasXSAVEOPT() { return bits(cpuid(0xd,1).eax,0); }
inline bool hasXSAVEC() { return bits(cpuid(0xd,1).eax,1); }
inline bool hasXSAVES() { return bits(cpuid(0xd,1).eax,3); }
inline bool hasNX() { return bits(cpuid(0x80000001,0).edx,20); }

/** the size (in bytes) required by XSAVES for state components corresponding the features XCR0 | IA32_XSS.
 * includes system state for supervisor operation. */
inline uint32_t get_xsaves_size() { return cpuid(0xd, 1).ebx; }

/** the size (in bytes) required by XSAVE for state components corresponding the features XCR0. */
inline uint32_t get_xsave_size(void) { return cpuid(0xd, 0).ebx; }

inline uint32_t getXFeatureOffset(uint8_t idx) { return cpuid(0xd, idx).ebx; }
inline uint32_t getXFeatureSize(uint8_t idx) { return cpuid(0xd, idx).eax; }
inline bool isXFeatureSupervisor(uint8_t idx) { return bits(cpuid(0xd, idx).ecx,0); }
inline bool isXFeatureAligned(uint8_t idx) { return bits(cpuid(0xd, idx).ecx,1); }

inline uint64_t xgetbv(uint32_t index) {
  uint32_t eax, edx;
  asm volatile(".byte 0x0f,0x01,0xd0" : "=a" (eax), "=d" (edx) : "c" (index));
  return (uint64_t(edx)<<32) | eax;
}

inline void xsetbv(uint32_t index, uint64_t value) {
  asm volatile(".byte 0x0f,0x01,0xd1" : : "a" (value), "d" (value>>32), "c" (index));
}

inline void fxsave(FXRegs* state) { asm volatile("fxsaveq %0" : "=m" (*state)); }
inline void fxrestore(FXRegs* state) { asm volatile("fxrstorq %0" : "=m" (*state)); }

inline void xsave(XRegs* state, uint64_t mask) {
  asm volatile(".byte 0x48,0x0f,0xae,0x27" : : "D" (state), "m" (*state), "a" (mask), "d" (mask>>32) : "memory");
}

inline void xsaveopt(XRegs* state, uint64_t mask) {
  asm volatile(".byte 0x48,0x0f,0xae,0x37" : : "D" (state), "m" (*state), "a" (mask), "d" (mask>>32) : "memory");
}

inline void xsaves(XRegs* state, uint64_t mask) {
  asm volatile(".byte 0x48,0x0f,0xc7,0x2f" : : "D" (state), "m" (*state), "a" (mask), "d" (mask>>32) : "memory");
}

inline void xrstor(XRegs* state, uint64_t mask) {
  asm volatile(".byte 0x48,0x0f,0xae,0x2f" : : "D" (state), "m" (*state), "a" (mask), "d" (mask>>32) : "memory");
}

inline void xrstors(XRegs* state, uint64_t mask) {
  asm volatile(".byte 0x48,0x0f,0xc7,0x1f" : : "D" (state), "m" (*state), "a" (mask), "d" (mask>>32) : "memory");
}

/** XSave features. */
BITFIELD_DEF(uint64_t, XFeature)
  BoolField<value_t, base_t, 0> fp;
  BoolField<value_t, base_t, 1> xmm;
  BoolField<value_t, base_t, 2> ymm;
  BoolField<value_t, base_t, 3> bndregs;
  BoolField<value_t, base_t, 4> bndcsr;
  BoolField<value_t, base_t, 5> opmask;
  BoolField<value_t, base_t, 6> zmm_hi256;
  BoolField<value_t, base_t, 7> hi16_zmm;
  BoolField<value_t, base_t, 8> pt;
  BoolField<value_t, base_t, 9> pkru;
  constexpr static uint8_t MAX = 10;
  bool enabled(uint8_t idx) const { return bits(value, idx); }
  bool fpsse() const { return fp && xmm; }
  bool avx512() const { return opmask && zmm_hi256 && hi16_zmm; }
BITFIELD_END

inline XFeature getXFeaturesMask() {
  auto r = cpuid(0xd, 0);
  return XFeature(uint64_t(r.edx) | r.eax);
}

// XCR0: XFEATURE_ENABLED_MASK
inline uint64_t getXCR0() { return xgetbv(0); }
inline void setXCR0(uint64_t value) { xsetbv(0, value); }
inline void setXCR0(XFeature v) { xsetbv(0, v.value); }

} // namespace x86
} // namespace mythos

