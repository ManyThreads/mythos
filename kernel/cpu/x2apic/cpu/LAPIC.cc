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
    MLOG_DETAIL(mlog::boot, "initializing local X2APIC");
    PANIC(x86::x2ApicSupported());
    x86::enableX2Apic();

    // init the local interrupt sources
    // all of them should be in invalid state anyway
    auto lvt = RegLVT().masked(1);
    x86::setMSR(REG_LVT_TIMER, lvt);
    x86::setMSR(REG_LVT_THERMAL, lvt);
    x86::setMSR(REG_LVT_PERFCNT, lvt);
    x86::setMSR(REG_LVT_LINT0, lvt);
    x86::setMSR(REG_LVT_LINT1, lvt);
    x86::setMSR(REG_LVT_ERROR, lvt);

    // enable all interrupts by task priority 0
    // see 10.8.6 Task Priority in IA-32e Mode
    // Loading the TPR with a task-priority class of 0 enables all external interrupts.
    // Loading the TPR with a task-priority class of 0FH (01111B) disables all external interrupts.
    // In 64-bit mode, software can read and write the TPR using the MOV CR8 instruction.
    x86::setMSR(REG_TPR, Register(x86::getMSR(REG_TPR)).task_prio_sub(0).task_prio(0));

    // init the Spurious Interrupt Vector Register for handling local interrupt sources
    // after the reset SPURINT == 0xFF, setting bit 8 enables the APIC, for example 0x100
    // the lowest 4 bits should be 0, the vector should be above 32 (processor internal interrupts)
    // but the acutal value does not matter, because it is configured for each separately...
    auto svr = Register(x86::getMSR(REG_SVR));
    svr.vector = 0xFF;  // this interrupt should never be triggered anyway
    svr.apic_enable = 1;
    svr.focus_processor_checking = 0;
    svr.eio_suppression = 0;
    x86::setMSR(REG_SVR, svr);

    // init Error register
    x86::setMSR(REG_ESR, 0); // Be paranoid about clearing APIC errors.
    x86::setMSR(REG_LVT_ERROR, RegLVT().masked(1).vector(0xFF));
    x86::setMSR(REG_ESR, 0); // spec says clear errors after enabling vector.
}

void X2Apic::setTimerCounter(uint32_t count)
{
    x86::setMSR(REG_TIMER_DCR, 0x0b);  // divide bus clock by 0xa=128 or 0xb=1
    x86::setMSR(REG_TIMER_ICR, count);
}

uint32_t X2Apic::getTimerCounter()
{
    return uint32_t(x86::getMSR(REG_TIMER_CCR));
}

void X2Apic::enableTimer(uint8_t irq, bool periodic) {
    MLOG_INFO(mlog::boot, "x2apic enable periodic timer", DVAR(irq), DVAR(periodic));
    RegLVT reg(x86::getMSR(REG_LVT_TIMER));
    reg = reg.timer_mode(periodic ? PERIODIC : ONESHOT).masked(0).vector(irq);
    x86::setMSR(REG_LVT_TIMER, reg);
}

void X2Apic::disableTimer() {
    MLOG_INFO(mlog::boot, "x2apic disable timer");
    RegLVT reg(x86::getMSR(REG_LVT_TIMER));
    x86::setMSR(REG_LVT_TIMER, reg.timer_mode(ONESHOT).masked(1).vector(0xFF));
}

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
    hwthread_wait(10000); // 10000us
    auto esr = x86::getMSR(REG_ESR);
    MLOG_DETAIL(mlog::boot, "x2apic broadcasted INIT", DVARhex(esr));

    // send edge-triggered SIPI
    ASSERT((startIP & 0x0FFF) == 0 && ((startIP >> 12) >> 8) == 0);
    x86::setMSR(REG_ESR, 0); // Be paranoid about clearing APIC errors.
    x86::setMSR(REG_ICR, RegICR().destination(0) // TODO or 0xFFFFFFFF ?
        .destination_shorthand(ICR_DESTSHORT_NOTSELF)
        .delivery_mode(MODE_SIPI).vector(startIP >> 12));
    esr = x86::getMSR(REG_ESR);
    MLOG_DETAIL(mlog::boot, "x2apic broadcasted SIPI", DVARhex(esr));
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
    hwthread_wait(10000); // 10000us
    auto esr = x86::getMSR(REG_ESR);
    MLOG_DETAIL(mlog::boot, "x2apic sent INIT", DVAR(apicid), DVARhex(esr));

    // send edge-triggered SIPI
    ASSERT((startIP & 0x0FFF) == 0 && ((startIP >> 12) >> 8) == 0);
    x86::setMSR(REG_ESR, 0); // Be paranoid about clearing APIC errors.
    x86::setMSR(REG_ICR, RegICR().destination(apicid)
        .delivery_mode(MODE_SIPI).vector(startIP>>12));
    esr = x86::getMSR(REG_ESR);
    MLOG_DETAIL(mlog::boot, "x2apic sent SIPI ", DVAR(apicid), DVARhex(esr));

    // a second edge might actually be needed, a single seems to be fine enough
    //hwthread_wait(200); // 200us
    //x86::setMSR(REG_ICR, RegICR().destination(apicid)
        //.delivery_mode(MODE_SIPI).vector(startIP>>12));
    //esr = x86::getMSR(REG_ESR);
    //MLOG_DETAIL(mlog::boot, "apic sent SIPI ", DVAR(apicid), DVARhex(esr));
}

void X2Apic::sendNMI(uint32_t apicid)
{
    x86::setMSR(REG_ESR, 0); // Be paranoid about clearing APIC errors.
    x86::setMSR(REG_ICR, RegICR().destination(apicid).delivery_mode(MODE_NMI));
    auto esr = x86::getMSR(REG_ESR);
    MLOG_DETAIL(mlog::boot, "x2apic sent SIPI ", DVAR(apicid), DVARhex(esr));
}

void X2Apic::sendIRQ(uint32_t apicid, uint8_t vector)
{
    MLOG_INFO(mlog::boot, "x2apic sending IRQ:", DVAR(apicid), DVAR(vector));
    x86::setMSR(REG_ICR, RegICR().destination(apicid).vector(vector));
}

void X2Apic::endOfInterrupt()
{
    x86::setMSR(REG_EOI, 0);
}

} // namespace mythos
