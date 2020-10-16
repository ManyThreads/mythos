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

#include "runtime/PortalBase.hh"
#include "mythos/protocol/RaplDriverIntel.hh"
#include "mythos/init.hh"

namespace mythos {

  class RaplDriverIntel : public KObject
  {
  public:

    RaplDriverIntel() {}
    RaplDriverIntel(CapPtr cap) : KObject(cap) {}

    struct Result : public RaplVal {
      Result() {}
      Result(InvocationBuf* ib) {
        auto val = ib->cast<protocol::RaplDriverIntel::Result>()->val;

        pp0 = val.pp0;
        pp1 = val.pp1;
        psys = val.psys;
        pkg = val.pkg;
        dram = val.dram;
        cpu_energy_units = val.cpu_energy_units;
        dram_energy_units = val.dram_energy_units;
      }
    };

    PortalFuture<Result>
    getRaplVal(PortalLock pr) {
      return pr.invoke<protocol::RaplDriverIntel::GetRaplVal>(_cap);
    }

  };

} // namespace mythos
