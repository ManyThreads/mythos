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

#include "mythos/protocol/common.hh"

namespace mythos {
  namespace protocol {

    struct ProcessorManagement {
      constexpr static uint8_t proto = PROCESSORMANAGEMENT;

      enum Methods : uint8_t {
        ALLOCCORE,
        RETALLOCCORE,
        FREECORE,
        RETFREECORE
      };

      struct AllocCore : public InvocationBase {
        constexpr static uint16_t label = (proto<<8) + ALLOCCORE;
        AllocCore() : InvocationBase(label,getLength(this)) {
        }
      };

      struct RetAllocCore : public InvocationBase {
        constexpr static uint16_t label = (proto<<8) + RETALLOCCORE;
        RetAllocCore(CapPtr sc) : InvocationBase(label,getLength(this)) {
          addExtraCap(sc);
        }

        CapPtr sc() const { return this->capPtrs[0]; }
      };

      struct FreeCore : public InvocationBase {
        constexpr static uint16_t label = (proto<<8) + FREECORE;
        FreeCore(CapPtr sc) : InvocationBase(label,getLength(this)) {
          addExtraCap(sc);
        }

        CapPtr sc() const { return this->capPtrs[0]; }
      };

      //struct RetFreeCore : public InvocationBase {
        //constexpr static uint16_t label = (proto<<8) + RETFREECORE;
        //Result() : InvocationBase(label,getLength(this)) {
        //}
      //};

      template<class IMPL, class... ARGS>
      static Error dispatchRequest(IMPL* obj, uint8_t m, ARGS const&...args) {
        switch(Methods(m)) {
          case ALLOCCORE: return obj->invoke_allocCore(args...);
          case FREECORE: return obj->invoke_freeCore(args...);
          default: return Error::NOT_IMPLEMENTED;
        }
      }

    };

  }// namespace protocol
}// namespace mythos
