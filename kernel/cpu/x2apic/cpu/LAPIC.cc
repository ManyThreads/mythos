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
 * Copyright 2019 Randolf Rotta and contributors, BTU Cottbus-Senftenberg
 */
#include "cpu/LAPIC.hh"
#include "cpu/hwthread_pause.hh"
#include "cpu/ctrlregs.hh"
#include "boot/mlog.hh"
#include "util/assert.hh"

namespace mythos {

X2Apic lapic;

uint32_t X2Apic::getId()
{
    return uint32_t(x86::getMSR(REG_APICID));
}

void X2Apic::init() {
    MLOG_DETAIL(mlog::boot, "initializing local x2-APIC");
    PANIC(x86::x2ApicSupported());
    // enable just x2apic and set bit 8 of the APIC spurious vector register (SVR)
    x86::enableX2Apic();
    x86::setMSR(REG_SVR, x86::getMSR(REG_SVR) | (1<<ENABLE_APIC));
}

//void LAPIC::enablePeriodicTimer(uint8_t irq, uint32_t count) {
    //MLOG_INFO(mlog::boot, "enable periodic APIC timer", DVAR(irq), DVAR(count));
    //write(REG_TIMER_DCR, 0x0b);  // divide bus clock by 0xa=128 or 0xb=1
    //setInitialCount(count);
    //write(REG_LVT_TIMER, read(REG_LVT_TIMER).timer_mode(PERIODIC).masked(0).vector(irq));
//}

//void LAPIC::disableTimer() {
    //MLOG_INFO(mlog::boot, "disable APIC timer");
    //write(REG_LVT_TIMER, read(REG_LVT_TIMER).timer_mode(ONESHOT).masked(1).vector(0));
//}

void X2Apic::startupBroadcast(size_t startIP)
{
    // send edge-triggered INIT
    x86::setMSR(REG_ESR, 0); // Be paranoid about clearing APIC errors.
    x86::setMSR(REG_ICR, RegICR().destination(0) // TODO or 0xFFFFFFFF ?
        .destination_shorthand(ICR_DESTSHORT_NOTSELF)
        .delivery_mode(MODE_INIT));
    // This delay must happen between `broadcastInitIPIEdge` and
    // `broadcastStartupIPI` in order for all hardware threads to
    // start on the real hardware.
    hwthread_wait(10000); // 10000 us ??
    auto esr = x86::getMSR(REG_ESR);
    MLOG_DETAIL(mlog::boot, "apic broadcasted INIT", DVARhex(esr));

    // send edge-triggered SIPI
    ASSERT((startIP & 0x0FFF) == 0 && ((startIP >> 12) >> 8) == 0);
    x86::setMSR(REG_ESR, 0); // Be paranoid about clearing APIC errors.
    x86::setMSR(REG_ICR, RegICR().destination(0) // TODO or 0xFFFFFFFF ?
        .destination_shorthand(ICR_DESTSHORT_NOTSELF)
        .delivery_mode(MODE_SIPI).vector(startIP >> 12));
    esr = x86::getMSR(REG_ESR);
    MLOG_DETAIL(mlog::boot, "apic broadcasted SIPI", DVARhex(esr));
}

void X2Apic::startup(uint32_t apicid, size_t startIP)
{
    // send edge-triggered INIT
    x86::setMSR(REG_ESR, 0); // Be paranoid about clearing APIC errors.
    x86::setMSR(REG_ICR, RegICR().destination(apicid)
        .delivery_mode(MODE_INIT));
    // This delay must happen between `broadcastInitIPIEdge` and
    // `broadcastStartupIPI` in order for all hardware threads to
    // start on the real hardware.
    hwthread_wait(10000); // 10000 us ??
    auto esr = x86::getMSR(REG_ESR);
    MLOG_DETAIL(mlog::boot, "apic sent INIT", DVAR(apicid), DVARhex(esr));

    // send edge-triggered SIPI
    ASSERT((startIP & 0x0FFF) == 0 && ((startIP >> 12) >> 8) == 0);
    x86::setMSR(REG_ESR, 0); // Be paranoid about clearing APIC errors.
    x86::setMSR(REG_ICR, RegICR().destination(apicid)
        .delivery_mode(MODE_SIPI).vector(startIP>>12));
    esr = x86::getMSR(REG_ESR);
    MLOG_DETAIL(mlog::boot, "apic sent SIPI ", DVAR(apicid), DVARhex(esr));

    // a second edge might actually be needed
    hwthread_wait(200); // 200 us ??
    x86::setMSR(REG_ICR, RegICR().destination(apicid)
        .delivery_mode(MODE_SIPI).vector(startIP>>12));
    esr = x86::getMSR(REG_ESR);
    MLOG_DETAIL(mlog::boot, "apic sent SIPI ", DVAR(apicid), DVARhex(esr));
}

void X2Apic::sendNMI(uint32_t apicid)
{
    x86::setMSR(REG_ESR, 0); // Be paranoid about clearing APIC errors.
    x86::setMSR(REG_ICR, RegICR().destination(apicid).delivery_mode(MODE_NMI));
    auto esr = x86::getMSR(REG_ESR);
    MLOG_DETAIL(mlog::boot, "apic sent SIPI ", DVAR(apicid), DVARhex(esr));
}

void X2Apic::sendIRQ(uint32_t apicid, uint8_t vector)
{
    MLOG_INFO(mlog::boot, "apic sending IRQ:", DVAR(apicid), DVAR(vector));
    x86::setMSR(REG_ICR, RegICR().destination(apicid).vector(vector));
}

void X2Apic::endOfInterrupt()
{
    x86::setMSR(REG_EOI, 0);
}

} // namespace mythos
