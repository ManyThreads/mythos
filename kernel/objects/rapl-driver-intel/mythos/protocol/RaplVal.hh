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

namespace mythos {
  struct RaplVal{
    RaplVal()
      : pp0(0)
      , pp1(0)
      , psys(0)
      , pkg(0)
      , dram(0)
      , cpu_energy_units(0)
      , dram_energy_units(0)
    {}

    RaplVal(uint64_t pp0, uint64_t pp1, uint64_t psys, uint64_t pkg, uint64_t dram, uint32_t cpu_energy_units, uint32_t dram_energy_units)
      : pp0(pp0)
      , pp1(pp1)
      , psys(psys)
      , pkg(pkg)
      , dram(dram)
      , cpu_energy_units(cpu_energy_units)
      , dram_energy_units(dram_energy_units)
    {}

    void add(RaplVal& rv){
      this->pp0 += rv.pp0;
      this->pp1 += rv.pp1;
      this->psys += rv.psys;
      this->pkg += rv.pkg;
      this->dram += rv.dram;
      ASSERT(this->cpu_energy_units == rv.cpu_energy_units);
      ASSERT(this->dram_energy_units == rv.dram_energy_units);
    }

    // rounded (truncated) number in Joule
    uint64_t getEnergyPP0(){ return pp0 >> cpu_energy_units; } 
    uint64_t getEnergyPP1(){ return pp1 >> cpu_energy_units; } 
    uint64_t getEnergyPSYS(){ return psys >> cpu_energy_units; } 
    uint64_t getEnergyPKG(){ return pkg >> cpu_energy_units; } 
    uint64_t getEnergyDRAM(){ return dram >> dram_energy_units; } 

    uint64_t pp0;
    uint64_t pp1;
    uint64_t psys;
    uint64_t pkg;
    uint64_t dram;
    uint32_t cpu_energy_units;
    uint32_t dram_energy_units;
  };
}// namespace mythos
