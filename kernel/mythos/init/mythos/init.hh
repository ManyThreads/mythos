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
 * Copyright 2016 Randolf Rotta, Robert Kuban, and contributors, BTU Cottbus-Senftenberg
 */
#pragma once

#include <cstddef>
#include "mythos/caps.hh"
#include "mythos/InvocationBuf.hh"

namespace mythos {
namespace init {

  enum CSpaceLayout : CapPtr {
    NULLCAP = 0,
    KM,
    CSPACE,
    PML4,
    EC,
    PORTAL,
    EXAMPLE_FACTORY,
    MEMORY_REGION_FACTORY,
    EXECUTION_CONTEXT_FACTORY,
    PORTAL_FACTORY,
    CAPMAP_FACTORY,
    PAGEMAP_FACTORY,
    UNTYPED_MEMORY_FACTORY,
    SIGNAL_LISTENER_FACTORY,
    CAP_ALLOC_START,
    CAP_ALLOC_END = CAP_ALLOC_START+200,
    MSG_FRAME,
    DEVICE_MEM,
    SCHEDULERS_START,
    CPUDRIVER = SCHEDULERS_START+256,
    RAPL_DRIVER_INTEL,
    PROCESSOR_ALLOCATOR,
    INFO_FRAME,
    INTERRUPT_CONTROL_START,
    INTERRUPT_CONTROL_END = INTERRUPT_CONTROL_START+256,
    APP_CAP_START = 1024,
    SIZE = 4096
  };

} // namespace init
} // namespace mythos
