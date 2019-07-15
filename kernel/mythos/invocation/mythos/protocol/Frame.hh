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

    struct MemoryRoot {
      constexpr static uint8_t proto = MEMORY_ROOT;

      enum Methods : uint8_t {
        CREATE
      };

      struct Create : public InvocationBase {
        constexpr static uint16_t label = (proto<<8) + CREATE;

        Create(CapPtr dst, size_t addr, size_t size, bool writable)
          : InvocationBase(label, getLength(this), 2)
          , dstDepth(0), addr(addr), size(size), writable(writable)
        {
          this->dstPtr = dst;
          this->factory(null_cap);
          this->dstSpace(null_cap);
        }

        CapPtr factory() const { return this->capPtrs[0]; }
        CapPtr dstSpace() const { return this->capPtrs[1]; }
        void factory(CapPtr c) { this->capPtrs[0] = c; }
        void dstSpace(CapPtr c) { this->capPtrs[1] = c; }

        void setIndirectDest(CapPtr dstCSpace, CapPtrDepth dstDepth) {
          this->dstSpace(dstCSpace);
          this->dstDepth = dstDepth;
        }

        CapPtrDepth dstDepth;
        uint64_t addr;
        uint64_t size;
        bool writable;
      };

      template<class IMPL, class... ARGS>
      static Error dispatchRequest(IMPL* obj, uint8_t m, ARGS const&...args) {
        switch(Methods(m)) {
          case CREATE: return obj->invokeCreate(args...);
          default: return Error::NOT_IMPLEMENTED;
        }
      }
    };

    struct Frame {
      constexpr static uint8_t proto = FRAME;

      enum Methods : uint8_t {
        INFO
      };

      BITFIELD_DEF(CapRequest, FrameReq)
      BoolField<value_t,base_t,0> writable;
      BoolField<value_t,base_t,1> kernel;
      UIntField<value_t,base_t,2,5> sizeBits;
      UIntField<value_t,base_t,7,25> offset;
      enum Sizes { PAGE_4KB = 4096, PAGE_2MB=4096*512, PAGE_1GB=4096*512*512 };

      FrameReq() : value(0) {}
      constexpr static size_t log2(size_t n) {
        return ( (n<2) ? 0 : 1+log2(n/2));
      }
      FrameReq size(size_t size) { return this->sizeBits(log2(size/4096)); }
      BITFIELD_END

      struct Info : public InvocationBase {
        constexpr static uint16_t label = (proto<<8) + INFO;
        Info() : InvocationBase(label,getLength(this)) {}

        uint64_t addr;
        uint64_t size;
        bool kernel;
        bool writable;
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
