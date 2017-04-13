/* -*- mode:C++; indent-tabs-mode:nil; -*- */
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
 * Copyright 2016 Randolf Rotta, Robert Kuban, and contributors, BTU Cottbus-Senftenberg
 */
#pragma once

#include "mythos/protocol/common.hh"

namespace mythos {
namespace protocol {

  struct UntypedMemory {
    constexpr static uint8_t proto = UNTYPED_MEMORY;

    enum Methods : uint8_t {
      PROPERTIES, // ??
      CREATE
    };

    // note: other concrete factory stubs will extend this message by additional caps and parameters
    struct CreateBase : public InvocationBase {
      constexpr static uint16_t label = (proto<<8) + CREATE;
      CreateBase(CapPtr dst, CapPtr factory) : InvocationBase(label,getLength(this)) {
        this->dstPtr = dst;
        addExtraCap(factory);
        addExtraCap(null_cap);
        this->dstDepth = 0;
      }

      CapPtr factory() const { return this->capPtrs[0]; }
      CapPtr dstSpace() const { return this->capPtrs[1]; }

      void setIndirectDest(CapPtr dstCSpace, CapPtrDepth dstDepth) {
        this->capPtrs[1] = dstCSpace;
        this->dstDepth = dstDepth;
      }

      CapPtrDepth dstDepth;
    };

    struct Create : public CreateBase {
      Create(CapPtr dst, CapPtr factory, size_t size, size_t alignment)
        : CreateBase(dst, factory), size(size), alignment(alignment)
      {}
      size_t size;
      size_t alignment;
    };

    template<class IMPL, class... ARGS>
    static Error dispatchRequest(IMPL* obj, uint8_t m, ARGS const&...args) {
      switch(Methods(m)) {
      case CREATE: return obj->invokeCreate(args...);
      default: return Error::NOT_IMPLEMENTED;
      }
    }
  };

} // namespace protocol
} // namespace mythos
