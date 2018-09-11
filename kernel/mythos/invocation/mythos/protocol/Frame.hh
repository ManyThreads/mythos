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
 * Copyright 2016 Randolf Rotta, Robert Kuban, and contributors, BTU Cottbus-Senftenberg
 */
#pragma once

#include "mythos/protocol/common.hh"
#include "mythos/protocol/KernelMemory.hh"
#include "util/error-trace.hh"

namespace mythos {
  namespace protocol {

    struct Frame {
      constexpr static uint8_t proto = FRAME;

      enum Methods : uint8_t {
        INFO
      };

      BITFIELD_DEF(CapRequest, FrameReq)
      BoolField<value_t,base_t,0> writable;
      UIntField<value_t,base_t,1,2> size;
      UIntField<value_t,base_t,3,29> offset;
      enum Sizes { PAGE_4KB = 0, PAGE_2MB, PAGE_1GB, PAGE_512GB };
      FrameReq() : value(0) {}
      BITFIELD_END

      struct Info : public InvocationBase {
        constexpr static uint16_t label = (proto<<8) + INFO;
        Info() : InvocationBase(label,getLength(this)) {}

        uint64_t addr;
        uint64_t size;
        bool writable;
        bool executable;
      };

      struct Create : public KernelMemory::CreateBase {
        Create(CapPtr dst, CapPtr factory, size_t size, size_t alignment)
          : CreateBase(dst, factory, getLength(this), 0), size(size), alignment(alignment)
        {}
        size_t size;
        size_t alignment;
      };

      template<class IMPL, class... ARGS>
      static Error dispatchRequest(IMPL* obj, uint8_t m, ARGS const&...args) {
        switch(Methods(m)) {
          case INFO: return obj->frameInfo(args...);
          default: return Error::NOT_IMPLEMENTED;
        }
      }
    };

  }// namespace protocol
}// namespace mythos
