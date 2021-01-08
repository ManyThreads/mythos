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
 * Copyright 2020 Philipp Gypser and contributors, BTU Cottbus-Senftenberg
 */
#pragma once

#include "async/NestedMonitorDelegating.hh"
#include "objects/IKernelObject.hh"

namespace mythos {

class RaplDriverIntel
  : public IKernelObject
{
public:
  optional<void const*> vcast(TypeId) const override { THROW(Error::TYPE_MISMATCH); }
  optional<void> deleteCap(CapEntry&, Cap, IDeleter&) override { RETURN(Error::SUCCESS); }
  void deleteObject(Tasklet*, IResult<void>*) override {}
  void invoke(Tasklet* t, Cap self, IInvocation* msg) override;

public:
  RaplDriverIntel();
  Error invoke_getRaplVal(Tasklet*, Cap, IInvocation* msg);
  void printEnergy();

private:
  bool isIntel;
  uint32_t cpu_fam;
  uint32_t cpu_model;
	uint32_t dram_avail;
	bool pp0_avail;
	bool pp1_avail;
	bool psys_avail;
	bool different_units;
  uint32_t power_units;
  uint32_t time_units;
  uint32_t cpu_energy_units;
  uint32_t dram_energy_units;
};

} // namespace mythos
