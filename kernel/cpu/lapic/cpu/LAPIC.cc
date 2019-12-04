/* -*- mode:C++; -*- */
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
 * Copyright 2014 Randolf Rotta, Maik Krüger, and contributors, BTU Cottbus-Senftenberg
 */
#include "cpu/LAPIC.hh"
#include "cpu/hwthread_pause.hh"
#include "cpu/ctrlregs.hh"
#include "boot/mlog.hh"
#include "util/assert.hh"

namespace mythos {
XApic lapic;

void XApic::initMSR()
{
    using namespace mythos::x86;
    if (x2ApicSupported()) {
        MLOG_DETAIL(mlog::boot, "detected x2Apic support");
        if (isX2ApicEnabled()) {
            MLOG_INFO(mlog::boot, "x2Apic is enabled, reset LAPIC by disabling");
            disableApic(); // disables both xAPIC and x2APIC
            enableApic(); // reenables xAPIC
            ASSERT(!isX2ApicEnabled());
        }
    }
}

void XApic::init() {
    MLOG_DETAIL(mlog::boot, "initializing local xAPIC");
    Register value;

    initMSR();

    // init the Destination Format Register and the Logical
    // Destination Register Intel recommends to set DFR, LDR and TPR
    // before enabling an APIC.  See e.g. "AP-388 82489DX User's
    // Manual" (Intel document number 292116).
    value = read(REG_DFR);
    value.model = 0xF; // flat mode
    write(REG_DFR, value);
    value = read(REG_LDR);
    value.destination = 1; // will not be used anyway
    write(REG_LDR, value);

    // init the local interrupt sources
    // all of them should be in invalid state anyway
    value.value = 0;
    value.masked = 1;
    write(REG_LVT_TIMER, value);
    write(REG_LVT_THERMAL, value);
    write(REG_LVT_PERFCNT, value);
    write(REG_LVT_LINT0, value);
    write(REG_LVT_LINT1, value);
    write(REG_LVT_ERROR, value);

    // enable all interrupts by task priority 0
    // see 10.8.6 Task Priority in IA-32e Mode
    // Loading the TPR with a task-priority class of 0 enables all external interrupts.
    // Loading the TPR with a task-priority class of 0FH (01111B) disables all external interrupts.
    // In 64-bit mode, software can read and write the TPR using the MOV CR8 instruction.
    value = read(REG_TASK_PRIO);
    value.task_prio_sub = 0;
    value.task_prio = 0;
    write(REG_TASK_PRIO, value);

    // After a crash, interrupts from previous run can still have ISR bit set.
    // Thus clear these with EndOfInterrupt.
    size_t queued = 0;
    for (size_t loops = 0; loops < 1000000; loops++) {
        for (size_t i = 0; i < 0x8; i++) queued |= read(REG_IRR + i * 0x10).value;
        if (!queued) break;
        for (size_t i = 0; i < 0x8; i++) {
            value = read(REG_ISR + i * 0x10);
            for (size_t j = 0; j < 32; j++) {
                if (value.value & (1 << j)) endOfInterrupt();
            }
        }
        if(loops%1000 == 0){MLOG_DETAIL(mlog::boot, "loop", DVAR(queued), DVAR(value.value));}
        queued = 0;
    }

    // init the Spurious Interrupt Vector Register for handling local interrupt sources
    // after the reset SPURINT == 0xFF, setting bit 8 enables the APIC, for example 0x100
    // the lowest 4 bits should be 0, the vector should be above 32 (processor internal interrupts)
    // but the acutal value does not matter, because it is configured for each separately...
    value = read(REG_SVR);
    value.vector = 0xFF;  // this interrupt should never be triggered anyway
    value.apic_enable = 1;
    value.focus_processor_checking = 0;
    value.eio_suppression = 0;
    write(REG_SVR, value);

    ASSERT(getId() == x86::initialApicID());

    // init Error register
    write(REG_ESR, 0); read(REG_ESR); // Due to the Pentium erratum 3AP.
    value = read(REG_LVT_ERROR);
    value.vector = 0xFF; // should never happen because we leave it masked
    value.masked = 1;
    write(REG_LVT_ERROR, value);
    // spec says clear errors after enabling vector.
    write(REG_ESR, 0); read(REG_ESR); // Due to the Pentium erratum 3AP.
}

void XApic::enablePeriodicTimer(uint8_t irq, uint32_t count) {
    MLOG_INFO(mlog::boot, "enable periodic APIC timer", DVAR(irq), DVAR(count));
    write(REG_TIMER_DCR, 0x0b);  // divide bus clock by 0xa=128 or 0xb=1
    setInitialCount(count);
    write(REG_LVT_TIMER, read(REG_LVT_TIMER).timer_mode(PERIODIC).masked(0).vector(irq));
}

void XApic::disableTimer() {
    MLOG_INFO(mlog::boot, "disable APIC timer");
    write(REG_LVT_TIMER, read(REG_LVT_TIMER).timer_mode(ONESHOT).masked(1).vector(0));
}

XApic::Register XApic::edgeIPI(IrcDestinationShorthand dest, IcrDeliveryMode mode, uint8_t vec) {
      return Register().destination_shorthand(dest).level_triggered(0).level(1)
        .logical_destination(0).delivery_mode(mode).vector(vec)
        .delivery_pending(0); //workaround for qemu
    }


void XApic::waitForIPI()
{
    while (read(REG_ICR_LOW).delivery_pending) hwthread_pause();
}

void XApic::startupBroadcast(size_t startIP)
{
    // send edge-triggered INIT
    write(REG_ESR, 0); // Be paranoid about clearing APIC errors.
    read(REG_ESR);
    writeIPI(0, edgeIPI(ICR_DESTSHORT_NOTSELF, MODE_INIT, 0));
    hwthread_wait(10000);
    // This delay must happen between `broadcastInitIPIEdge` and
    // `broadcastStartupIPI` in order for all hardware threads to
    // start on the real hardware.
    hwthread_wait(10000); // 10000 us ??
    auto esr = read(REG_ESR).value;
    MLOG_DETAIL(mlog::boot, "apic broadcasted INIT", DVARhex(esr));

    // send edge-triggered SIPI
    ASSERT((startIP & 0x0FFF) == 0 && ((startIP >> 12) >> 8) == 0);
    write(REG_ESR, 0); // Be paranoid about clearing APIC errors.
    read(REG_ESR);
    writeIPI(0, edgeIPI(ICR_DESTSHORT_NOTSELF, MODE_SIPI, uint8_t(startIP >> 12)));
    esr = read(REG_ESR).value;
    MLOG_DETAIL(mlog::boot, "SIPI broadcast result", DVARhex(esr));
}

void XApic::startup(uint32_t apicid, size_t startIP)
{
    // send edge-triggered INIT
    write(REG_ESR, 0); // Be paranoid about clearing APIC errors.
    read(REG_ESR);
    const auto assertINIT = Register()
      .destination_shorthand(ICR_DESTSHORT_NO)
      .level_triggered(0).level(0) // 1, 1 for level triggered
      .logical_destination(0)
      .delivery_mode(MODE_INIT).vector(0)
      .delivery_pending(0);
    writeIPI(apicid, assertINIT);
    waitForIPI();
    // This delay must happen between `broadcastInitIPIEdge` and
    // `broadcastStartupIPI` in order for all hardware threads to
    // start on the real hardware.
    hwthread_wait(10000); // 10000 us ??
    auto esr = read(REG_ESR).value;
    MLOG_DETAIL(mlog::boot, "apic sent INIT", DVAR(apicid), DVARhex(esr));

    //const auto deassertINIT = assertINIT.level(0);
    //writeIPI(apicid, deassertINIT);
    //waitForIPI();

    // send edge-triggered SIPI
    ASSERT((startIP & 0x0FFF) == 0 && ((startIP >> 12) >> 8) == 0);
    write(REG_ESR, 0); // Be paranoid about clearing APIC errors.
    read(REG_ESR);
    writeIPI(apicid, edgeIPI(ICR_DESTSHORT_NO, MODE_SIPI, uint8_t(startIP >> 12)));
    esr = read(REG_ESR).value;
    MLOG_DETAIL(mlog::boot, "apic sent SIPI", DVAR(apicid), DVARhex(esr));

    // a second edge might actually be needed
    hwthread_wait(200); // 200 us ??
    waitForIPI();
    writeIPI(apicid, edgeIPI(ICR_DESTSHORT_NO, MODE_SIPI, uint8_t(startIP >> 12)));
    esr = read(REG_ESR).value;
    MLOG_DETAIL(mlog::boot, "apic sent SIPI", DVAR(apicid), DVARhex(esr));
}

void XApic::sendNMI(uint32_t apicid) {
    write(REG_ESR, 0); // Be paranoid about clearing APIC errors.
    read(REG_ESR);
    writeIPI(apicid, edgeIPI(ICR_DESTSHORT_NO, MODE_NMI, 0));
    write(REG_ESR, 0); // Be paranoid about clearing APIC errors.
}

void XApic::sendIRQ(uint32_t apicid, uint8_t vector)
{
    MLOG_INFO(mlog::boot, "send IRQ:", DVAR(apicid), DVAR(vector));
    //write(REG_ESR, 0); // Be paranoid about clearing APIC errors.
    //read(REG_ESR);
    writeIPI(apicid, edgeIPI(ICR_DESTSHORT_NO, MODE_FIXED, vector));
    //write(REG_ESR, 0); // Be paranoid about clearing APIC errors.
}

void XApic::writeIPI(size_t destination, Register icrlow) {
    ASSERT(destination < 256);
    MLOG_DETAIL(mlog::boot, "write ICR", DVAR(destination), DVARhex(icrlow.value));

    waitForIPI();

    write(REG_ICR_HIGH, Register().destination(destination));
    write(REG_ICR_LOW, icrlow);
}

} // namespace mythos
