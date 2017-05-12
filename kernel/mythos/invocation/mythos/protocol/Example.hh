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

#include <cstring>
#include "mythos/protocol/common.hh"
#include "mythos/protocol/KernelMemory.hh"

namespace mythos {
  namespace protocol {

    struct Example {
      constexpr static uint8_t proto = EXAMPLE;

      enum Methods : uint8_t {
        PRINT_MESSAGE,
		PING,
		MOVE_HOME,
      };

      struct PrintMessage : public InvocationBase {
        typedef InvocationBase response_type;
        constexpr static uint16_t label = (proto<<8) + PRINT_MESSAGE;
        PrintMessage(char const* str, size_t bytes)
          : InvocationBase(label,getLength(this))
        {
          if (bytes>InvocationBase::maxBytes) bytes = InvocationBase::maxBytes;
          this->bytes = uint16_t(bytes);
          this->tag.length = uint8_t((bytes+3)/4);
          memcpy(message, str, bytes);
        }
        uint16_t bytes;
        char message[InvocationBase::maxBytes-2];
      };

      struct Create : public KernelMemory::CreateBase {
        Create(CapPtr dst, CapPtr factory) : CreateBase(dst, factory) {}
      };

      struct Ping : public InvocationBase {
    	typedef InvocationBase response_type;
    	constexpr static uint16_t label = (proto<<8) + PING;
    	Ping(size_t wait_cycles): InvocationBase(label,getLength(this)){
    	  this->wait_cycles = uint64_t(wait_cycles);
    	  this->place = 1234567;
    	};
    	uint64_t wait_cycles;
    	uint64_t place;
      };

      struct MoveHome : public InvocationBase {
    	typedef InvocationBase response_type;
    	constexpr static uint16_t label = (proto<<8) + MOVE_HOME;
    	MoveHome(size_t location): InvocationBase(label,getLength(this)){
    	  this->location = uint16_t(location);
    	};
    	uint16_t location;
      };

      template<class IMPL, class... ARGS>
      static Error dispatchRequest(IMPL* obj, uint8_t m, ARGS const&...args) {
        switch(Methods(m)) {
          case PRINT_MESSAGE: return obj->printMessage(args...);
          case PING: return obj->ping(args...);
          case MOVE_HOME: return obj->moveHome(args...);
          default: return Error::NOT_IMPLEMENTED;
        }
      }

    };

  } // namespace protocol
} // namespace mythos
