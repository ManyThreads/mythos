/* -*- mode:C++; -*- */
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

    struct PageMap
    {
      constexpr static uint8_t proto = PAGEMAP;

      enum Methods : uint8_t {
        MAP,
        REMAP,
        PROTECT,
        UNMAP,
        INSTALLMAP,
        REMOVEMAP,
        RESULT
      };

      BITFIELD_DEF(CapRequest, PageMapReq)
      BoolField<value_t,base_t,0> configurable;
      PageMapReq() : value(0) {}
      BITFIELD_END

      BITFIELD_DEF(uint8_t, MapFlags)
      BoolField<value_t,base_t,0> writable;
      BoolField<value_t,base_t,1> executable;
      BoolField<value_t,base_t,2> cache_disabled;
      BoolField<value_t,base_t,3> write_through;
      BoolField<value_t,base_t,4> configurable;
      MapFlags() : value(0) {}
      BITFIELD_END

      struct InstallMap : public InvocationBase {
        constexpr static uint16_t label = (proto << 8) + INSTALLMAP;
        InstallMap(CapPtr pagemap, uintptr_t vaddr, size_t level, MapFlags flags)
	  : InvocationBase(label, getLength(this)), vaddr(vaddr), level(level), flags(flags)
	{
          addExtraCap(pagemap);
        }
        CapPtr pagemap() const { return capPtrs[0]; }
        uintptr_t vaddr;
        size_t level;
        MapFlags flags;
      };

      struct RemoveMap : public InvocationBase {
        constexpr static uint16_t label = (proto << 8) + REMOVEMAP;
        RemoveMap(uintptr_t vaddr, size_t level)
	  : InvocationBase(label, getLength(this)), vaddr(vaddr), level(level)
        {}
        uintptr_t vaddr;
        size_t level;
      };

      struct Mmap : public InvocationBase {
        constexpr static uint16_t label = (proto << 8) + MAP;
        Mmap(CapPtr frame, uintptr_t vaddr, size_t size, MapFlags flags)
	  : InvocationBase(label, getLength(this)), vaddr(vaddr), size(size), flags(flags)
	{
          addExtraCap(frame);
        }
        CapPtr tgtFrame() const { return capPtrs[0]; }
        uintptr_t vaddr;
	size_t size;
        MapFlags flags;
      };

      struct Remap : public InvocationBase {
        constexpr static uint16_t label = (proto << 8) + REMAP;
        Remap(uintptr_t sourceAddr, uintptr_t destAddr, size_t size)
	  : InvocationBase(label, getLength(this)), sourceAddr(sourceAddr),
	    destAddr(destAddr), size(size)
	{}
	uintptr_t sourceAddr;
	uintptr_t destAddr;
	size_t size;
      };

      struct Mprotect : public InvocationBase {
        constexpr static uint16_t label = (proto << 8) + PROTECT;
        Mprotect(uintptr_t vaddr, size_t size, MapFlags flags)
	  : InvocationBase(label, getLength(this)), vaddr(vaddr), size(size), flags(flags)
	{}
        uintptr_t vaddr;
	size_t size;
	MapFlags flags;
      };

      struct Munmap : public InvocationBase {
        constexpr static uint16_t label = (proto << 8) + UNMAP;
        Munmap(uintptr_t vaddr, size_t size)
	  : InvocationBase(label, getLength(this)), vaddr(vaddr), size(size)
	{}
        uintptr_t vaddr;
	size_t size;
      };

      struct Create : public KernelMemory::CreateBase {
        Create(CapPtr dst, CapPtr factory, size_t level)
          : CreateBase(dst, factory, getLength(this), 0), level(level)
        {}
        size_t level;
      };

      struct Result : public InvocationBase {
        constexpr static uint16_t label = (proto << 8) + RESULT;
        Result(uintptr_t vaddr, size_t size, size_t level)
	  : InvocationBase(label, getLength(this)), vaddr(vaddr), size(size), level(level)
	{}
        uintptr_t vaddr;
	size_t size;
        size_t level;
      };

      template<class IMPL, class... ARGS>
      static Error dispatchRequest(IMPL* obj, uint8_t m, ARGS const&...args) {
	switch(Methods(m)) {
	case MAP: return obj->invokeMmap(args...);
	case REMAP: return obj->invokeRemap(args...);
	case UNMAP: return obj->invokeMunmap(args...);
	case PROTECT: return obj->invokeMprotect(args...);
        case INSTALLMAP: return obj->invokeInstallMap(args...);
        case REMOVEMAP: return obj->invokeRemoveMap(args...);
	default: return Error::NOT_IMPLEMENTED;
	}
      }
    };

  } // namespace protocol
} // namespace mythos


