/* -*- mode:C++; -*- */
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
 * Copyright 2014 Randolf Rotta, Maik Krüger, and contributors, BTU Cottbus-Senftenberg
 */
#pragma once

#include <cstdint>
#include "util/streambuf.hh"
#include "util/mstring.hh" // for memcpy, strlen

namespace mythos {

  class MemStreamBuf final
    : public streambuf
  {
  public:
    MemStreamBuf(char* buf, size_t size)
      : _buf(buf), _used(0), _size(size)
    {
      sync();
    }

    virtual ~MemStreamBuf() {}

    size_t size() const { return _used; }
    char const* c_str() const { return _buf; }
    char* c_str() { return _buf; }

    int sputc(char c) {
      if (_used+1<_size) _buf[_used++] = c;
      return c;
    }

    mythos::streamsize sputn(char const* s, mythos::streamsize count) {
      size_t len = (_used + count < _size) ? count : _size - _used -1;
      memcpy(_buf+_used, s, len);
      _used += len;
      return len;
    }

    void putcstr(char const* s) { sputn(s, strlen(s)); }

    template<size_t N>
    void putcstr(const char (&s)[N]) { sputn(s, N); }

  protected:
    void sync() { _buf[_used] = 0; }

  protected:
    virtual int _sputc(char c) { return sputc(c); }
    virtual mythos::streamsize _sputn(char const* s, mythos::streamsize count) { return sputn(s,count); }
    virtual void _putcstr(char const* s) { putcstr(s); }
    virtual void _sync() { sync(); }

  protected:
    char* _buf;
    size_t _used;
    size_t _size;
  };

} // namespace mythos
