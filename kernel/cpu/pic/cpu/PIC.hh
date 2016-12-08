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
 * Copyright 2014 Randolf Rotta, Maik Krüger, and contributors, BTU Cottbus-Senftenberg
 */
#pragma once

#include "cpu/hwthread_pause.hh"

namespace mythos {
  namespace cpu {

/** Disable the 8259 PIC properly. 
 *
 * (From http://wiki.osdev.org/APIC)
 * This is nearly as important as setting up the APIC. You do this
 * in two steps: masking all interrupts and remapping the
 * IRQs. Masking all interrupts disables them in the
 * PIC. Remapping is what you probably already did when you used
 * the PIC: you want interrupt requests to start at 32 instead of
 * 0 to avoid conflicts with the exceptions. This is necessary
 * because even though you masked all interrupts on the PIC, it
 * could still give out spurious interrupts which will then be
 * misinterpreted from your kernel as exceptions.
 */
inline void disablePIC()
{
  asm volatile("outb %0, $0x20" : : "r"(char(0x11)));
  hwthread_pause();
  asm volatile("outb %0, $0xa0" : : "r"(char(0x11)));
  hwthread_pause();
  asm volatile("outb %0, $0x21" : : "r"(char(0x20)));
  hwthread_pause();
  asm volatile("outb %0, $0xa1" : : "r"(char(0x28)));
  hwthread_pause();
  asm volatile("outb %0, $0x21" : : "r"(char(0x04)));
  hwthread_pause();
  asm volatile("outb %0, $0xa1" : : "r"(char(0x02)));
  hwthread_pause();
  asm volatile("outb %0, $0x21" : : "r"(char(0x01)));
  hwthread_pause();
  asm volatile("outb %0, $0xa1" : : "r"(char(0x01)));
  hwthread_pause();
  // Hardware-Interrupts durch PICs ausmaskieren. Nur der Interrupt 2,
  // der der Kaskadierung der beiden PICs dient, ist erlaubt.
  asm volatile("outb %0, $0xa1" : : "r"(char(0xff)));
  hwthread_pause();
  asm volatile("outb %0, $0x21" : : "r"(char(0xfb)));
  hwthread_pause();
}

  } // namespace cpu
} // namespace mythos
