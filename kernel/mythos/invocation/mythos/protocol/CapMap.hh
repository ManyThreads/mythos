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

namespace mythos {
  namespace protocol {

    struct CapMap {
      constexpr static uint8_t proto = CAPMAP;

      enum Methods : uint8_t {
        DERIVE,
        REFERENCE,
        MOVE,
        DELETE,
        REVOKE
      };

      struct BinaryOp : public InvocationBase {

        uint8_t srcDepth;
        uint8_t dstDepth;

        CapPtr dstCS() const { return this->capPtrs[0]; }
        CapPtr srcPtr() const { return this->capPtrs[1]; }
        CapPtr dstPtr() const { return this->capPtrs[2]; }

      protected:
        BinaryOp(uint16_t label, uint8_t length,
            CapPtr src, uint8_t srcDepth,
            CapPtr dstCS, CapPtr dst, uint8_t dstDepth)
          : InvocationBase(label, length)
          , srcDepth(srcDepth), dstDepth(dstDepth)
        {
          addExtraCap(dstCS);
          addExtraCap(src);
          addExtraCap(dst);
        }
      };

      struct Derive : public BinaryOp {
        constexpr static uint16_t label = (proto<<8) + DERIVE;

        Derive(CapPtr src, uint8_t srcDepth,
            CapPtr dstCS, CapPtr dst, uint8_t dstDepth,
            CapRequest req)
          : BinaryOp(label, getLength(this), src, srcDepth, dstCS, dst, dstDepth)
          , request(req)
        {}

        CapRequest request;
      };

      struct Reference : public BinaryOp {
        constexpr static uint16_t label = (proto<<8) + REFERENCE;

        Reference(CapPtr src, uint8_t srcDepth,
            CapPtr dstCS, CapPtr dst, uint8_t dstDepth,
            CapRequest req)
          : BinaryOp(label, getLength(this), src, srcDepth, dstCS, dst, dstDepth)
          , request(req)
        {}

        CapRequest request;
      };

      struct Move : public BinaryOp {
        constexpr static uint16_t label = (proto<<8) + MOVE;

        Move(CapPtr src, uint8_t srcDepth,
            CapPtr dstCS, CapPtr dst, uint8_t dstDepth)
          : BinaryOp(label, getLength(this), src, srcDepth, dstCS, dst, dstDepth)
        {}

      };

      struct Delete : public InvocationBase {
        constexpr static uint16_t label = (proto<<8) + DELETE;

        Delete(CapPtr src, uint8_t srcDepth)
          : InvocationBase(label, getLength(this))
          , srcDepth(srcDepth)
        {
          addExtraCap(src);
        }

        CapPtr srcPtr() const { return this->capPtrs[0]; }
        uint8_t srcDepth;
      };

      struct Revoke : public InvocationBase {
        constexpr static uint16_t label = (proto<<8) + REVOKE;

        Revoke(CapPtr src, uint8_t srcDepth)
          : InvocationBase(label, getLength(this))
          , srcDepth(srcDepth)
        {
          addExtraCap(src);
        }

        CapPtr srcPtr() const { return this->capPtrs[0]; }
        uint8_t srcDepth;
      };

      struct Create : public KernelMemory::CreateBase {
        Create(CapPtr dst, CapPtr factory, CapPtrDepth indexbits, CapPtrDepth guardbits, CapPtr guard)
          : CreateBase(dst, factory, getLength(this), 0), indexbits(indexbits), guardbits(guardbits), guard(guard)
        {}
        CapPtrDepth indexbits;
        CapPtrDepth guardbits;
        CapPtr guard;
      };

      template<class IMPL, class... ARGS>
        static Error dispatchRequest(IMPL* obj, uint8_t m, ARGS const&...args) {
          switch(Methods(m)) {
            case REFERENCE: return obj->invokeReference(args...);
            case DERIVE: return obj->invokeDerive(args...);
            case MOVE: return obj->invokeMove(args...);
            case DELETE: return obj->invokeDelete(args...);
            case REVOKE: return obj->invokeRevoke(args...);
            default: return Error::NOT_IMPLEMENTED;
          }
        }

      /*
      template<class IMPL, class... ARGS>
        static Error dispatchResponse(IMPL* obj, uint8_t m, ARGS const&...args) {
          switch(Methods(m)) {
            case WRITE_REGISTERS: return obj->readRegisters(args...); // write is the answer to read :(
            default: return Error::NOT_IMPLEMENTED;
          }
        }
      */

    };

  } // namespace protocol
}
