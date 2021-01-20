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
 * Copyright 2021 Philipp Gypser, BTU Cottbus-Senftenberg
 */
#include "plugins/test-perfmon.hh"

#include "cpu/hwthreadid.hh"
#include "cpu/perfmon.hh"

namespace mythos {

namespace test_perfmon {

TestPerfMon instance;

TestPerfMon::TestPerfMon()
  : TestPlugin("test performance monitoring:")
{}

void TestPerfMon::initGlobal()
{
}

void TestPerfMon::initThread(cpu::ThreadID threadID)
{
  if (threadID == 0) {
    runTest();
  }
}

bool primeTest(uint64_t n){

  if(n == 0 || n == 1){
    return false;
  }

  for(uint64_t i = 2; i <= n / 2; i++){
    if(n % i == 0) return false;
  }

  return true;
}

void TestPerfMon::runTest(){
  x86::PerformanceMonitoring pm;
  if(!pm.leafAvail()){
    log.error("ERROR: performance monitoring leave in in cpuid not available");
    return;
  }
  log.error("leaf in in cpuid available");

  auto version = pm.version();
  log.error(DVAR(version));

  auto numGenPur = pm.numGenPurpPerfMon();
  log.error("number of general purpose counters:", numGenPur);

  auto GenPurwidth = pm.genPurpPerfMonBitWidth();
  log.error("bit width of general purpose counters:", GenPurwidth);

  auto eventVectorLength = pm.perfMonEventVectorLength();
  log.error("event vector length:", eventVectorLength);

  for(unsigned e = 0; e < eventVectorLength; e++){
    log.error("Event ", e, pm.eventAvail(e)? " is available :D" : " is not available :`(");
  }

  for(uint8_t i = 0; i < 16; i++){
    log.error(DVAR(i), DVAR(pm.fixedCounterSupported(i)));
  }

  auto numberFixedFunPCs = pm.numberFixedFunPCs();
  log.error(DVAR(numberFixedFunPCs));

  auto bitWidthFixedFunPCs = pm.bitWidthFixedFunPCs();
  log.error(DVAR(bitWidthFixedFunPCs));

  pm.activateAllFFCS();
  pm.barrier();
  auto instrs1 = pm.readInstRetired();
  auto unhalted1 = pm.readCyclesUnhalted();
  auto cycles1 = pm.readRefCycles();
  pm.barrier();
  
  unsigned numPrimes = 0;
  const uint64_t max = 20000;

  for(uint64_t i = 0; i < max; i++){
    if(primeTest(i)) numPrimes++;
  }

  pm.barrier();
  auto instrs2 = pm.readInstRetired();
  auto unhalted2 = pm.readCyclesUnhalted();
  auto cycles2 = pm.readRefCycles();
  pm.barrier();

  auto instrs = instrs2-instrs1;
  auto unhalted = unhalted2-unhalted1;
  auto cycles = cycles2-cycles1;

  log.error("Primes found: ", DVAR(numPrimes));
  log.error("fixed function counters: ", DVAR(instrs), DVAR(unhalted), DVAR(cycles));
}

} // test_perfmon
} // mythos

