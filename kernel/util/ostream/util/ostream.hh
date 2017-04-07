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

#include "util/streambuf.hh"
#include <cstdint>
#include <cstddef>

namespace mythos {

  template<class STREAM=streambuf>
  class ostream_base
  {
  public:
    typedef STREAM stream_t;
    ostream_base() : base(10), padding(0), padchar('0'), buf(nullptr) {}
    ostream_base(stream_t* buf) : base(10), padding(0), padchar('0'), buf(buf) {}
    //ostream(ostream const& o) : base(o.base), buf(o.buf) {}

    void setStreamBuf(stream_t* buf) { this->buf = buf; }
    stream_t* getStreamBuf() const { return this->buf; }

    ostream_base& put(char c) { buf->sputc(c); return *this; }
    ostream_base& write(char const* s, streamsize count) { buf->sputn(s, count); return *this; }
    ostream_base& flush() { buf->pubsync(); return *this; }

    ostream_base& operator<< (unsigned char c) { return format(c); }
    ostream_base& operator<< (unsigned short i) { return format(i); }
    ostream_base& operator<< (unsigned int i) { return format(i); }
    ostream_base& operator<< (unsigned long i) { return format(i); }
    ostream_base& operator<< (unsigned long long i) { return format(i); }
    ostream_base& operator<< (char c) { return put(c); }
    ostream_base& operator<< (short i) { return signedFormat(i); }
    ostream_base& operator<< (int i) { return signedFormat(i); }
    ostream_base& operator<< (long i) { return signedFormat(i); }
    ostream_base& operator<< (long long i) { return signedFormat(i); }
    ostream_base& operator<< (void const* i) { return write("0x",2).format(uintptr_t(i), 16,2*sizeof(i)); }

    template<class... ARGS>
    ostream_base& operator<< (void (*i)(ARGS...)) { return write("0x",2).format(uintptr_t(i), 16,2*sizeof(i)); }

    ostream_base& operator<< (char const* c) { buf->putcstr(c); return *this; }

    template<size_t N>
    ostream_base& operator<< (char const (&c)[N]) { buf->putcstr(c); return *this; }

    ostream_base& operator<< (ostream_base& (*func)(ostream_base&)) { return func(*this); }

    size_t base;
    unsigned padding;
    char padchar;

    ostream_base& format(uint64_t val, size_t base, size_t padding, char padchar);
    ostream_base& format(uint64_t val, size_t base, size_t padding) {
      return format(val, base, padding, padchar);
    }
    ostream_base& format(uint64_t val, size_t base) { return format(val, base, padding, padchar); }
    ostream_base& format(uint64_t val) { return format(val, base, padding, padchar); }

    ostream_base& signedFormat(int64_t val) {
      return (val<0) ? put('-').format(uint64_t(-val)) : format(uint64_t(val));
    }

  protected:
    stream_t* buf;
  };

  template<class STREAM>
  ostream_base<STREAM>& ostream_base<STREAM>::format(uint64_t val, size_t base,
                 size_t padding, char padchar)
  {
    const size_t maxlen = 65;
    const char tab[] = "0123456789abcdef";
    char buffer[maxlen];
    size_t i=0;

    if(val == 0) {
      buffer[maxlen-i-1] = '0';
      i++;
    } else {
      while(val != 0 && i<maxlen) {
        buffer[maxlen-i-1] = tab[val % base];
        val /= base;
        i++;
      }
    }

    while (i<padding && i<maxlen) {
      buffer[maxlen-i-1] = padchar;
      i++;
    }
    write(buffer+maxlen-i, i);

    return *this;
  }

  template<class STREAM>
  inline ostream_base<STREAM>& endl(ostream_base<STREAM>& os) { return os.put('\n').flush(); }

  template<class STREAM>
  inline ostream_base<STREAM>& dec(ostream_base<STREAM>& os) { os.base=10; return os; }

  template<class STREAM>
  inline ostream_base<STREAM>& hex(ostream_base<STREAM>& os) { os.base=16; return os; }

  template<class STREAM>
  inline ostream_base<STREAM>& oct(ostream_base<STREAM>& os) { os.base=8; return os; }

  template<class STREAM>
  inline ostream_base<STREAM>& bin(ostream_base<STREAM>& os) { os.base=2; return os; }

  typedef ostream_base<streambuf> ostream;

} // namespace mythos
