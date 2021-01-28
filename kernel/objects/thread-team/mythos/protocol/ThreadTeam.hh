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
#pragma once

#include "mythos/protocol/common.hh"
#include "mythos/protocol/KernelMemory.hh"
#include "util/align.hh"


namespace mythos {
  namespace protocol {

    struct ThreadTeam {
      constexpr static uint8_t proto = THREADTEAM;

      enum Methods : uint8_t {
        TRYRUNEC,
        RETTRYRUNEC,
        DEMANDRUNEC,
        RETDEMANDRUNEC,
        FORCERUNEC,
        RETFORCERUNEC,
        RUNNEXTTOEC,
        RETRUNNEXTTOEC
      };

      struct Create : public KernelMemory::CreateBase {
        typedef InvocationBase response_type;
        //constexpr static uint16_t label = (proto<<8) + CREATE;
        Create(CapPtr dst, CapPtr factory, CapPtr pa)
          : CreateBase(dst, factory, getLength(this), 0)
        {
          this->capPtrs[2] = pa;
        }
        CapPtr pa() const { return this->capPtrs[2]; }
      };

      struct TryRunEC : public InvocationBase {
        constexpr static uint16_t label = (proto<<8) + TRYRUNEC;
        TryRunEC(CapPtr ec) : InvocationBase(label,getLength(this)) {
          addExtraCap(ec);
        }

        // execution context to be scheduled
        CapPtr ec() const { return this->capPtrs[0]; }
      };

      struct RetTryRunEC : public InvocationBase {
        constexpr static uint16_t label = (proto<<8) + RETTRYRUNEC;
        RetTryRunEC() : InvocationBase(label,getLength(this)) {
        }
      };

      struct DemandRunEC : public InvocationBase {
        constexpr static uint16_t label = (proto<<8) + DEMANDRUNEC;
        DemandRunEC(CapPtr ec) : InvocationBase(label,getLength(this)) {
          addExtraCap(ec);
        }

        // execution context to be scheduled
        CapPtr ec() const { return this->capPtrs[0]; }
      };

      struct RetDemandRunEC : public InvocationBase {
        constexpr static uint16_t label = (proto<<8) + RETDEMANDRUNEC;
        RetDemandRunEC() : InvocationBase(label,getLength(this)) {
        }
      };

      struct ForceRunEC : public InvocationBase {
        constexpr static uint16_t label = (proto<<8) + FORCERUNEC;
        ForceRunEC(CapPtr ec) : InvocationBase(label,getLength(this)) {
          addExtraCap(ec);
        }

        // execution context to be scheduled
        CapPtr ec() const { return this->capPtrs[0]; }
      };

      struct RetForceRunEC : public InvocationBase {
        constexpr static uint16_t label = (proto<<8) + RETFORCERUNEC;
        RetForceRunEC() : InvocationBase(label,getLength(this)) {
        }
      };

      struct RunNextToEC : public InvocationBase {
        constexpr static uint16_t label = (proto<<8) + RUNNEXTTOEC;
        RunNextToEC(CapPtr ec, CapPtr ec_place) : InvocationBase(label,getLength(this)) {
          addExtraCap(ec);
          addExtraCap(ec_place);
        }

        // execution context to be scheduled
        CapPtr ec() const { return this->capPtrs[0]; }
        CapPtr ec_place() const { return this->capPtrs[1]; }
      };

      struct RetRunNextToEC : public InvocationBase {
        constexpr static uint16_t label = (proto<<8) + RETRUNNEXTTOEC;
        RetRunNextToEC() : InvocationBase(label,getLength(this)) {
        }
      };

      template<class IMPL, class... ARGS>
      static Error dispatchRequest(IMPL* obj, uint8_t m, ARGS const&...args) {
        switch(Methods(m)) {
          case TRYRUNEC: return obj->invokeTryRunEC(args...);
          case DEMANDRUNEC: return obj->invokeDemandRunEC(args...);
          case FORCERUNEC: return obj->invokeForceRunEC(args...);
          case RUNNEXTTOEC: return obj->invokeRunNextToEC(args...);
          default: return Error::NOT_IMPLEMENTED;
        }
      }

    };

  }// namespace protocol
}// namespace mythos
