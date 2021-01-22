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

    struct ProcessorAllocator {
      constexpr static uint8_t proto = PROCESSORALLOCATOR;

      enum Methods : uint8_t {
        ALLOC,
        RETALLOC,
        FREE,
        RETFREE
      };

      struct Alloc : public InvocationBase {
        constexpr static uint16_t label = (proto<<8) + ALLOC;
        Alloc(CapPtr dstMap = null_cap) : InvocationBase(label,getLength(this)) {
          addExtraCap(dstMap);
        }

        // target cap map
        CapPtr dstSpace() const { return this->capPtrs[0]; }
      };

      struct RetAlloc : public InvocationBase {
        constexpr static uint16_t label = (proto<<8) + RETALLOC;
        RetAlloc(CapPtr sc) : InvocationBase(label,getLength(this)) {
          addExtraCap(sc);
        }

        // allocated scheduling context
        CapPtr sc() const { return this->capPtrs[0]; }
      };

      struct Free : public InvocationBase {
        constexpr static uint16_t label = (proto<<8) + FREE;
        Free(CapPtr sc) : InvocationBase(label,getLength(this)) {
          addExtraCap(sc);
        }

        // scheduling context to be freed
        CapPtr sc() const { return this->capPtrs[0]; }
      };

      struct RetFree : public InvocationBase {
        constexpr static uint16_t label = (proto<<8) + RETFREE;
        RetFree() : InvocationBase(label,getLength(this)) {
        }
      };

      template<class IMPL, class... ARGS>
      static Error dispatchRequest(IMPL* obj, uint8_t m, ARGS const&...args) {
        switch(Methods(m)) {
          case ALLOC: return obj->invokeAlloc(args...);
          case FREE: return obj->invokeFree(args...);
          default: return Error::NOT_IMPLEMENTED;
        }
      }

    };

  }// namespace protocol
}// namespace mythos
