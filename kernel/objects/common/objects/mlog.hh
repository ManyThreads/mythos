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
#pragma once

#include "util/Logger.hh"

namespace mlog {

#ifndef MLOG_CAP
#define MLOG_CAP FilterWarning
#endif
  extern Logger<MLOG_CAP> cap;

#ifndef MLOG_SCHED
#define MLOG_SCHED FilterError
#endif
  extern Logger<MLOG_SCHED> sched;

#ifndef MLOG_SYSCALL
#define MLOG_SYSCALL FilterWarning
#endif
  extern Logger<MLOG_SYSCALL> syscall;

#ifndef MLOG_EC
#define MLOG_EC FilterWarning
#endif
  extern Logger<MLOG_EC> ec;

#ifndef MLOG_KM
#define MLOG_KM FilterWarning
#endif
  extern Logger<MLOG_KM> km;

#ifndef MLOG_IRQ
#define MLOG_IRQ FilterWarning
#endif
  extern Logger<MLOG_IRQ> irq;

} // namespace mlog
