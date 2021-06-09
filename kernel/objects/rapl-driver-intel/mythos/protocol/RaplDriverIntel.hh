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
#include "mythos/protocol/RaplVal.hh"

namespace mythos {
  namespace protocol {

    struct RaplDriverIntel {
      constexpr static uint8_t proto = RAPLDRIVERINTEL;

      enum Methods : uint8_t {
        GETRAPLVAL,
        GETGLOBALRAPLVAL,
        RESULT
      };

      struct GetRaplVal : public InvocationBase {
        constexpr static uint16_t label = (proto<<8) + GETRAPLVAL;
        GetRaplVal() : InvocationBase(label,getLength(this)) {
        }
      };

      struct GetGlobalRaplVal : public InvocationBase {
        constexpr static uint16_t label = (proto<<8) + GETGLOBALRAPLVAL;
        GetGlobalRaplVal() : InvocationBase(label,getLength(this)) {
        }
      };

      struct Result : public InvocationBase {
        constexpr static uint16_t label = (proto<<8) + RESULT;
        Result() : InvocationBase(label,getLength(this)) {
        }
        RaplVal val;
      };

      template<class IMPL, class... ARGS>
      static Error dispatchRequest(IMPL* obj, uint8_t m, ARGS const&...args) {
        switch(Methods(m)) {
          case GETRAPLVAL: return obj->invoke_getRaplVal(args...);
          case GETGLOBALRAPLVAL: return obj->invoke_getGlobalRaplVal(args...);
          default: return Error::NOT_IMPLEMENTED;
        }
      }

    };

  }// namespace protocol
}// namespace mythos
