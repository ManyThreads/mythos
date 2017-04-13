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

namespace mythos {

  template<class OS1=streambuf, class OS2=streambuf>
  class TeeStreamBuf 
    : public streambuf
  {
  public:
    TeeStreamBuf(OS1* b1, OS2* b2) 
      : b1(b1), b2(b2)
    {
    }
    virtual ~TeeStreamBuf() {}

    void setOut1(OS1* b) { b1 = b; }
    void setOut2(OS2* b) { b2 = b; }

    int sputc(char c) {
      if (b1) b1->sputc(c);
      if (b2) b2->sputc(c);
      return c;
    }

    mythos::streamsize sputn(char const* s, mythos::streamsize count) {
      if (b1) b1->sputn(s,count);
      if (b2) b2->sputn(s,count);
      return count;	   
    }

    void putcstr(char const* s) {
      if (b1) b1->putcstr(s);
      if (b2) b2->putcstr(s);
    }

  protected:
    void sync() {
      if (b1) b1->pubsync();
      if (b2) b2->pubsync();
    }

  protected:
    virtual int _sputc(char c) { return sputc(c); }
    virtual mythos::streamsize _sputn(char const* s, mythos::streamsize count) { return sputn(s,count); }
    virtual void _putcstr(char const* s) { putcstr(s); }
    virtual void _sync() { sync(); }

  protected:
    OS1* b1;
    OS2* b2;
  };


  template<class OS1=streambuf, class OS2=streambuf>
  TeeStreamBuf<OS1,OS2> tee(OS1* b1, OS2* b2) {
    return TeeStreamBuf<OS1,OS2>(b1,b2);
  }
  
} // namespace mythos
