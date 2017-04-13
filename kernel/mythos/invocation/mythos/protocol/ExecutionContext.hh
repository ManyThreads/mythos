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

#include "mythos/protocol/UntypedMemory.hh"
#include "mythos/protocol/common.hh"

namespace mythos {
  namespace protocol {

    struct ExecutionContext {
      constexpr static uint8_t proto = EXECUTION_CONTEXT;

      enum Methods : uint8_t {
        CONFIGURE,
        READ_REGISTERS,
        WRITE_REGISTERS,
        SET_FSGS,
        RESUME,
        SUSPEND
      };

      struct Configure : public InvocationBase {
        typedef InvocationBase response_type;
        constexpr static uint16_t label = (proto<<8) + CONFIGURE;
        Configure(CapPtr as, CapPtr cs, CapPtr sched)
          : InvocationBase(label,getLength(this))
        {
          addExtraCap(as);
          addExtraCap(cs);
          addExtraCap(sched);
        }

        CapPtr as() const { return this->capPtrs[0]; }
        CapPtr cs() const { return this->capPtrs[1]; }
        CapPtr sched() const { return this->capPtrs[2]; }
      };

      struct Amd64Registers {
        uint64_t rax, rbx, rcx, rdx, rdi, rsi;
        uint64_t r8, r9, r10, r11, r12, r13, r14, r15;
        uint64_t rip, rsp, rbp;
        uint64_t fs_base, gs_base;
      };

      // has WriteRegisters message as result
      struct ReadRegisters : public InvocationBase {
        constexpr static uint16_t label = (proto<<8) + READ_REGISTERS;
        ReadRegisters(bool suspend)
          : InvocationBase(label,getLength(this)), suspend(suspend) {}
        bool suspend;
      };

      struct WriteRegisters : public InvocationBase {
        constexpr static uint16_t label = (proto<<8) + WRITE_REGISTERS;
        WriteRegisters(bool resume, Amd64Registers regs = Amd64Registers())
          : InvocationBase(label,getLength(this)), resume(resume), regs(regs) {}
        bool resume;
        Amd64Registers regs;
      };

      struct SetFSGS : public InvocationBase {
        constexpr static uint16_t label = (proto<<8) + SET_FSGS;
        SetFSGS(uintptr_t fs, uintptr_t gs)
          : InvocationBase(label,getLength(this)), fs(fs), gs(gs) {}
        uintptr_t fs;
        uintptr_t gs;
      };

      struct Resume : public InvocationBase {
        constexpr static uint16_t label = (proto<<8) + RESUME;
        Resume() : InvocationBase(label,getLength(this)) {}
      };

      struct Suspend : public InvocationBase {
        constexpr static uint16_t label = (proto<<8) + SUSPEND;
        Suspend() : InvocationBase(label,getLength(this)) {}
      };

      struct Create : public UntypedMemory::CreateBase {
        typedef InvocationBase response_type;
        Create(CapPtr dst, CapPtr factory, CapPtr as, CapPtr cs, CapPtr sched, Amd64Registers regs, bool start)
          : CreateBase(dst, factory), regs(regs), start(start)
        {
          addExtraCap(as);
          addExtraCap(cs);
          addExtraCap(sched);
        }
        Create(CapPtr dst, CapPtr factory, CapPtr as, CapPtr cs, CapPtr sched, bool start)
          : CreateBase(dst, factory), start(start)
        {
          addExtraCap(as);
          addExtraCap(cs);
          addExtraCap(sched);
        }
        Amd64Registers regs;
        bool start;
        CapPtr as() const { return this->capPtrs[2]; }
        CapPtr cs() const { return this->capPtrs[3]; }
        CapPtr sched() const { return this->capPtrs[4]; }
      };

      template<class IMPL, class... ARGS>
      static Error dispatchRequest(IMPL* obj, uint8_t m, ARGS const&...args) {
        switch(Methods(m)) {
        case CONFIGURE: return obj->invokeConfigure(args...);
        case READ_REGISTERS: return obj->invokeReadRegisters(args...);
        case WRITE_REGISTERS: return obj->invokeWriteRegisters(args...);
        case SET_FSGS: return obj->invokeSetFSGS(args...);
        case RESUME: return obj->invokeResume(args...);
        case SUSPEND: return obj->invokeSuspend(args...);
        default: return Error::NOT_IMPLEMENTED;
        }
      }
    };

  } // namespace protocol
} // namespace mythos
