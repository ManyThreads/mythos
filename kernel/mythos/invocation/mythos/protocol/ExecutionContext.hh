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

#include "mythos/protocol/KernelMemory.hh"
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
        SUSPEND,
        SET_PRIORITY
      };

      enum Signals : uint64_t {
        NO_SIGNAL = 0ull,
        // first 0x1F = 31 signals are reserved for traps/interrupts
        // we might change that later if we need the space
        // signal number is 1ull << (#IRC);
        TRAP_DIV_BY_ZERO = 1ull << 0,
        TRAP_SINGLE_STEP = 1ull << 1,
        TRAP_NMI         = 1ull << 2,
        TRAP_BREAKPOINT  = 1ull << 3,
        // ...
        TRAP_PAGEFAULT   = 1ull << 14,
        // ...
        // other signals start from bit 32
        TRAP_EXIT        = 1ull << 32,
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
          Amd64Registers() {}
        uint64_t rax = 0;
        uint64_t rbx = 0;
        uint64_t rcx = 0;
        uint64_t rdx = 0;
        uint64_t rdi = 0;
        uint64_t rsi = 0;
        uint64_t r8 = 0;
        uint64_t r9 = 0;
        uint64_t r10 = 0;
        uint64_t r11 = 0;
        uint64_t r12 = 0;
        uint64_t r13 = 0;
        uint64_t r14 = 0;
        uint64_t r15 = 0;
        uint64_t rip = 0;
        uint64_t rsp = 0;
        uint64_t rbp = 0;
        uint64_t fs_base = 0;
        uint64_t gs_base = 0;
        uint64_t irq = 0; // read-only
        uint64_t error = 0; // read-only
        uint64_t cr2 = 0; // read-only
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

      struct SetPriority : public InvocationBase {
        constexpr static uint16_t label = (proto<<8) + SET_PRIORITY;
        SetPriority(bool priority)
          : InvocationBase(label,getLength(this)), priority(priority) {}
        bool priority;
      };

      struct Resume : public InvocationBase {
        constexpr static uint16_t label = (proto<<8) + RESUME;
        Resume() : InvocationBase(label,getLength(this)) {}
      };

      struct Suspend : public InvocationBase {
        constexpr static uint16_t label = (proto<<8) + SUSPEND;
        Suspend() : InvocationBase(label,getLength(this)) {}
      };

      struct Create : public KernelMemory::CreateBase {
        typedef InvocationBase response_type;
        Create(CapPtr dst, CapPtr factory) 
          : CreateBase(dst, factory, getLength(this), 3), start(false) { }
        Amd64Registers regs;
        bool start;
        CapPtr as() const { return this->capPtrs[2]; }
        CapPtr cs() const { return this->capPtrs[3]; }
        CapPtr sched() const { return this->capPtrs[4]; }
        void as(CapPtr c) { this->capPtrs[2] = c; }
        void cs(CapPtr c) { this->capPtrs[3] = c; }
        void sched(CapPtr c) { this->capPtrs[4] =c; }
      };

      template<class IMPL, class... ARGS>
      static Error dispatchRequest(IMPL* obj, uint8_t m, ARGS const&...args) {
        switch(Methods(m)) {
        case CONFIGURE: return obj->invokeConfigure(args...);
        case READ_REGISTERS: return obj->invokeReadRegisters(args...);
        case WRITE_REGISTERS: return obj->invokeWriteRegisters(args...);
        case SET_FSGS: return obj->invokeSetFSGS(args...);
        case SET_PRIORITY: return obj->invokeSetPriority(args...);
        case RESUME: return obj->invokeResume(args...);
        case SUSPEND: return obj->invokeSuspend(args...);
        default: return Error::NOT_IMPLEMENTED;
        }
      }
    };

  } // namespace protocol
} // namespace mythos
