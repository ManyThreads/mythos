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
 * Copyright 2014 Randolf Rotta, Maik Krüger, and contributors, BTU Cottbus-Senftenberg
 */
#pragma once

#include "util/ostream.hh"
#include "util/PhysPtr.hh"
#include "util/hash.hh"
#include "util/Range.hh"
#include <cstring>

namespace mlog {

  using mythos::ostream_base;

  template<typename T, size_t N>
  struct DebugVar
  {
    DebugVar(char const (&name)[N], T const& value) : name(name), value(value) {}
    char const (&name)[N];
    T const& value;
  };

  template<class S, typename T, size_t N>
  ostream_base<S>& operator<< (ostream_base<S>& out, DebugVar<T,N> const& v) {
    out << v.name << "=" << v.value; return out;
  }

  template<class S, typename T, size_t N>
  ostream_base<S>& operator<< (ostream_base<S>& out, DebugVar<T*,N> const& v) {
    out << v.name << "=" << (void const*)v.value; return out;
  }

#define DVAR(var) mlog::DebugVar<decltype(var),sizeof(#var)>(#var,var)

  template<typename T>
  struct DebugVarHex
  {
    DebugVarHex(char const* name, T const& value) : name(name), value(value) {}
    char const* name;
    T const& value;
  };

  template<class S, typename T>
  inline ostream_base<S>& operator<< (ostream_base<S>& out, DebugVarHex<T> const& v) {
    out << v.name << "=0x" << mythos::hex << v.value << mythos::dec;
    return out;
  }

  template<class S, typename T>
  inline ostream_base<S>& operator<< (ostream_base<S>& out, DebugVarHex<T*> const& v) {
    out << v.name << "=" << (void const*)v.value; return out;
  }

  template<class S, class T>
  inline ostream_base<S>& operator<< (ostream_base<S>& out,
				      DebugVarHex<mythos::PhysPtr<T>> const& v) {
    out << v.name << "=" << v.value.physint(); return out;
  }

  template<class S, class T>
  inline ostream_base<S>& operator<< (ostream_base<S>& out,
				      DebugVarHex<mythos::PhysPtr32<T>> const& v) {
    out << v.name << "=" << v.value.physint(); return out;
  }

#define DVARhex(var) mlog::DebugVarHex<decltype(var)>(#var,var)

  struct DebugMemRange
  {
    DebugMemRange(size_t start, size_t length) : start(start), length(length) {}
    template<class S>
    friend ostream_base<S>& operator<< (ostream_base<S>& out, DebugMemRange const& v) {
      out << (void*)v.start << "-" << (void*)(v.start+v.length) << " ";
      if (v.length >= 1024*1024*1024)
        out << v.length/1024/1024/1024 << "GiB";
      else if (v.length >= 1024*1024)
        out << v.length/1024/1024 << "MiB";
      else if (v.length >= 1024)
        out << v.length/1024 << "KiB";
      else
        out << v.length << "B";
      return out;
    }
    size_t start;
    size_t length;
  };

#define DMRANGE(start, length) mlog::DebugMemRange(start,length)

  struct DebugString
  {
    DebugString(char const* start, size_t length) : start(start), length(length) {}
    template<class S>
    friend ostream_base<S>& operator<< (ostream_base<S>& out, DebugString const& v) {
      return out.write(v.start, v.length);
    }
    char const* start;
    size_t length;
  };

#define DSTR(cstr) mlog::DebugCString(cstr)

  struct DebugCString
  {
    DebugCString(char const* start) : start(start) {}
    template<class S>
    friend ostream_base<S>& operator<< (ostream_base<S>& out, DebugCString const& v) {
        if (v.start == nullptr) return out << "nullptr" ;
        else return out.write(v.start, strlen(v.start));
    }
    char const* start;
  };

  struct DebugMemDump
  {
    DebugMemDump(void* start, size_t length) : start(start), length(length) {}
    template<class S>
    friend ostream_base<S>& operator<< (ostream_base<S>& out, DebugMemDump const& v) {
      auto buf = reinterpret_cast<uint8_t*>(v.start);
      if (v.length) out.format(buf[0], 16, 2, '0');
      for (size_t i = 1; i < v.length; ++i)
      {
        out << ' ';
        out.format(buf[i], 16, 2, '0');
      }
      return out;
    }
    void* start;
    size_t length;
  };

#define DMDUMP(start, length) mlog::DebugMemDump(start,length)

  struct DebugMemHash
  {
    DebugMemHash(void* start, size_t length) : start(start), length(length) {}
    template<class S>
    friend ostream_base<S>& operator<< (ostream_base<S>& out, DebugMemHash const& v) {
      out.format(mythos::hash16(v.start, v.length), 16, 4, '0');
      return out;
    }
    void* start;
    size_t length;
  };

#define DMHASH(start, length) mlog::DebugMemHash(start,length)

} // namespace mlog
