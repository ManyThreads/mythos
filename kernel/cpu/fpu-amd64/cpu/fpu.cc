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
#include "cpu/fpu.hh"
#include "boot/mlog.hh"
#include <cstddef> // for offsetof

namespace mythos {
namespace cpu {

static x86::FpuState fpu_init_state;
static uint32_t fpu_xstate_size = 0;

static uint32_t mxcsr_feature_mask = 0;
static x86::XFeature xfeatures_mask = 0;

static uint32_t xstate_offsets[x86::XFeature::MAX];
static uint32_t xstate_sizes[x86::XFeature::MAX];
static uint32_t xstate_comp_offsets[x86::XFeature::MAX];

enum FpuMode { FSAVE, FXSAVE, XSAVE, XSAVEOPT, XSAVES };
static char const * const fpuModeNames[] = 
  {"FSAVE", "FXSAVE", "XSAVE", "XSAVEOPT", "XSAVES"};
static FpuMode fpu_mode = FSAVE;


static void fpu_setup_xstate()
{
  // ask cpuid for supported features
  xfeatures_mask = x86::getXFeaturesMask(); 
  mlog::boot.detail("xsave features from cpuid 0xd", DVARhex(xfeatures_mask));
  // and then disable features not supported by the processor
  if (!x86::hasFPU()) xfeatures_mask.fp = false;
  if (!x86::hasMMX()) xfeatures_mask.xmm = false;
  if (!x86::hasAVX()) xfeatures_mask.ymm = false;
  if (!x86::hasMPX()) {
    xfeatures_mask.bndregs = false;
    xfeatures_mask.bndcsr = false;
  }
  if (!x86::hasAVX512F()) {
    xfeatures_mask.opmask = false;
    xfeatures_mask.zmm_hi256 = false;
    xfeatures_mask.hi16_zmm = false;
  }
  if (!x86::hasINTELPT()) xfeatures_mask.pt = false;
  if (!x86::hasPKU()) xfeatures_mask.pkru = false;
  mlog::boot.detail("xsave features filtered by processor support", DVARhex(xfeatures_mask));
  // disable features not supported by mythos
  xfeatures_mask.pt = false;
  xfeatures_mask.pkru = false;
  mlog::boot.detail("xsave features filtered by mythos support", DVARhex(xfeatures_mask));

  PANIC(xfeatures_mask.fpsse()); // at least legacy FPU and MMX/SSE should be present
  x86::setXCR0(xfeatures_mask); // set XCR_XFEATURE_ENABLED_MASK

  // compute the context size for enabled features
  if (x86::hasXSAVES()) {
    fpu_xstate_size = x86::get_xsaves_size(); // xsave+Supervisor
    fpu_mode = XSAVES;
  } else if (x86::hasXSAVEOPT()) {
    fpu_xstate_size = x86::get_xsave_size();
    fpu_mode = XSAVEOPT;
  } else {
    fpu_xstate_size = x86::get_xsave_size();
    fpu_mode = XSAVE;
  }
  PANIC(fpu_xstate_size <= sizeof(x86::FpuState));
  //do_extra_xstate_size_checks();
  // for PT: update_regset_xstate_info(fpu_user_xstate_size,	xfeatures_mask & ~XFEATURE_MASK_SUPERVISOR);

  // get the offsets and sizes of the state space
  xstate_offsets[0] = 0; // legacy FPU state
  xstate_sizes[0] = offsetof(x86::FXRegs, xmm_space);
  xstate_offsets[1] = xstate_sizes[0]; // legacy MMX/SSE state
  xstate_sizes[1] = sizeof(x86::FXRegs::xmm_space);
  // YMM=2, XFEATURE_BNDREGS, XFEATURE_BNDCSR, XFEATURE_OPMASK, XFEATURE_ZMM_Hi256, XFEATURE_Hi16_ZMM,
  // XFEATURE_PT, XFEATURE_PKRU
  auto last_good_offset = offsetof(x86::XRegs, extended_state_area); // beginnning of the "extended state"
  for (uint8_t idx = 2; idx < x86::XFeature::MAX; idx++) {
    if (!xfeatures_mask.enabled(idx)) continue;
    xstate_offsets[idx] = (!x86::isXFeatureSupervisor(idx)) ? x86::getXFeatureOffset(idx) : 0; 
    xstate_sizes[idx] = x86::getXFeatureSize(idx);
    OOPS_MSG(last_good_offset <= xstate_offsets[idx], "cpu has misordered xstate");
    last_good_offset = xstate_offsets[idx];
  }

  // compute the offsets for the compacted format
  uint32_t xstate_comp_sizes[x86::XFeature::MAX];
  xstate_comp_offsets[0] = 0;
  xstate_comp_sizes[0] = xstate_sizes[0];
  xstate_comp_offsets[1] = offsetof(x86::FXRegs, xmm_space);
  xstate_comp_sizes[1] = xstate_sizes[1];
  if (!x86::hasXSAVES()) {
    for (uint8_t idx = 2; idx < x86::XFeature::MAX; idx++) {
      if (!xfeatures_mask.enabled(idx)) continue;
      xstate_comp_offsets[idx] = xstate_offsets[idx];
      xstate_comp_sizes[idx] = xstate_sizes[idx];
    }
  } else {
    xstate_comp_offsets[2] = 512+64; // FXSAVE_SIZE + XSAVE_HDR_SIZE
    for (uint8_t idx = 2; idx < x86::XFeature::MAX; idx++) {
      xstate_comp_sizes[idx] = xfeatures_mask.enabled(idx) ? xstate_sizes[idx] : 0;
      if (idx > 2) {
        xstate_comp_offsets[idx] = xstate_comp_offsets[idx-1] + xstate_comp_sizes[idx-1];
        if (x86::isXFeatureAligned(idx)) xstate_comp_offsets[idx] = (xstate_comp_offsets[idx]+63)/64*64; // round up
      }
    }
  }

  char const* names[] = {"x87", "mmx/sse", "ymm", "bndregs", "bndcsr", "opmask", "zmm_hi256", "hi16_zmm", "pt", "pkru"};
  for (uint8_t idx = 0; idx < x86::XFeature::MAX; idx++) {
    if (!xfeatures_mask.enabled(idx)) continue;
    mlog::boot.detail("we support XSAVE feature", idx, names[idx], 
      DVARhex(xstate_offsets[idx]), DVAR(xstate_sizes[idx]),
      DVARhex(xstate_comp_offsets[idx]), DVAR(xstate_comp_sizes[idx]));
  }
}

void FpuState::initBSP()
{
  initAP();

  mlog::boot.info("has x87 FPU:", DVAR(x86::hasMMX()), DVAR(x86::hasAVX()),
    DVAR(x86::hasAVX()&&x86::hasAVX512F()), DVAR(x86::hasXSAVE()),
    DVAR(x86::hasXSAVE()&&x86::hasXSAVEC()), 
    DVAR(x86::hasXSAVE()&&x86::hasXSAVEOPT()), 
    DVAR(x86::hasXSAVE()&&x86::hasXSAVES()));

  // initialise MXCR feature mask for fxsr
  if (x86::hasFXSR()) {
    x86::fxsave(&fpu_init_state.fxsave);
    if (fpu_init_state.fxsave.mxcsr_mask)  mxcsr_feature_mask = fpu_init_state.fxsave.mxcsr_mask;
    else mxcsr_feature_mask =  0x0000ffbf; // default mask: all features, except denormals-are-zero
  } else mxcsr_feature_mask = 0;

  // find the size needed to store the FPU state
  if (x86::hasXSAVE()) {
    fpu_setup_xstate();
  } else if (x86::hasFXSR()) {
    fpu_xstate_size = sizeof(x86::FXRegs);
    fpu_mode = FXSAVE;
  } else {
    fpu_xstate_size = sizeof(x86::FRegs);
    fpu_mode = FSAVE;
  }

  mlog::boot.info("fpu state storage will use", fpuModeNames[fpu_mode],
    DVAR(fpu_xstate_size), DVARhex(xfeatures_mask.value));
  PANIC(fpu_xstate_size <= sizeof(x86::FpuState));

  // set up the initial FPU context
  memset(&fpu_init_state, 0, fpu_xstate_size);
  if (x86::hasXSAVE()) {
    fpu_init_state.xsave.i387.cwd = 0x37f;
    fpu_init_state.xsave.i387.mxcsr = 0x1f80; // MXCSR_DEFAULT
    // Init all the features state with header.xfeatures being 0x0. This triggers xrstor to initialize the FPU.
    // Then dump the initialized state and store it as template for new execution contexts.
    if (x86::hasXSAVES()) {
      // XRSTORS requires these bits set in xcomp_bv, otherwisw #GP
      fpu_init_state.xsave.header.xcomp_bv = (uint64_t(1) << 63) | xfeatures_mask;
      x86::xrstors(&fpu_init_state.xsave, -1);
      x86::xsaves(&fpu_init_state.xsave, -1);
    } else if (x86::hasXSAVE()) {
      x86::xrstor(&fpu_init_state.xsave, -1);
      x86::xsave(&fpu_init_state.xsave, -1);
    }
  } else if (x86::hasFXSR()) {
    fpu_init_state.fxsave.cwd = 0x37f;
    fpu_init_state.fxsave.mxcsr = 0x1f80; // MXCSR_DEFAULT
  } else {
    fpu_init_state.fsave.cwd = 0xffff037fu;
    fpu_init_state.fsave.swd = 0xffff0000u;
    fpu_init_state.fsave.twd = 0xffffffffu;
    fpu_init_state.fsave.fos = 0xffff0000u;
  }
}

void FpuState::initAP()
{
  // assume presence of CPUID, if no CPUID, could try fpu__probe_without_cpuid() but we don't care yet
  PANIC(x86::hasFPU()); // has built-in FPU?

  // enable FXSAVE and FXRSTORE if supported; enable unmasked SSE exceptions
  auto cr4 = x86::getCR4();
  if (x86::hasFXSR()) cr4 |= x86::OSFXSR; // X86_FEATURE_FXSR
  if (x86::hasXMMEXCPT()) cr4 |= x86::OSXMMEXCPT; // X86_FEATURE_XMM aka SSE
  x86::setCR4(cr4);

  // clear TS (Task Switched) and EM (FPU Emulation)
  x86::setCR0(x86::getCR0() & ~(x86::TS | x86::EM));

  // flush all fpu state
  asm volatile ("clts");
  asm volatile ("fninit");

  // enable the extended processor state save/restore if XSAVE is supported
  if (x86::hasXSAVE()) {
    x86::setCR4(x86::getCR4() | x86::OSXSAVE); // enable XSAVE, xsetbv etc
    // Don't do this when called from initBSP() with uninitialized xfeatures_mask.
    // initAP() will called again on the BSP after starting all APs and, then, xfeatures_mask is usable.
    // xfeatures_mask is initialized by fpu_setup_xstate().
    if (xfeatures_mask.value) x86::setXCR0(xfeatures_mask); // XCR_XFEATURE_ENABLED_MASK
  }
}


void FpuState::save()
{
  switch (fpu_mode) {
  case XSAVES:
    // This saves system state when in supervisor mode.
    // Outside, its advantage is compacted storage of the actually modified state (combining XSAVEOPT+XSAVEC)
    x86::xsaves(&state.xsave, -1); // xsave_S_
    break;
  case XSAVEOPT: // this produces standard layout, just saves state that was modified since the last restore
    x86::xsaveopt(&state.xsave, -1);
    break;
  case XSAVE: // this produces standard layout
    x86::xsave(&state.xsave, -1);
    break;
  case FXSAVE: // legacy
    x86::fxsave(&state.fxsave);
    break;
  case FSAVE: // even more ancient, FNSAVE always clears FPU registers such that the FPU is unusable until FRSTOR !!!
    asm volatile("fnsave %0 ; fwait" : "=m" (state.fsave));
    break;
  };
}


void FpuState::restore()
{
  // The xrstors and xrstor initialize any fpu state that was not saved in the xsave extended state area.
  // This is stored as bitmask in the XSTATE_BV field of the xsave header.
  switch (fpu_mode) {
  case XSAVES: // this requires compacted layout as produced by XSAVES and XSAVEC
    x86::xrstors(&state.xsave, -1); // xrstor_S_
    break;
  case XSAVEOPT: // this requires standard layout
  case XSAVE:
    x86::xrstor(&state.xsave, -1);
    break;
  case FXSAVE: // legacy
    x86::fxrestore(&state.fxsave);
    break;
  case FSAVE: // even more ancient
    asm volatile("frstor %[fp]" :  "=m" (state.fsave) : [fp] "m" (state.fsave));
    break;
  };
}

void FpuState::clear()
{
  memcpy(&state, &fpu_init_state, fpu_xstate_size);
}

} // namespace cpu
} // namespace mythos


