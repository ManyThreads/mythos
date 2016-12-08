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

#include <cstddef>

namespace mythos {

  typedef size_t streamsize;

  /** similar to std::streambuf. This interface uses the virtual
   * method trampoline for more efficient composition.
   *
   * TODO implement static fixed sized buffer like in std::streambuf
   */
  class streambuf
  {
  public:
    virtual ~streambuf() {}

    int sputc(char c) { return _sputc(c); }
    mythos::streamsize sputn(char const* s, mythos::streamsize count) { return _sputn(s,count); }
    void putcstr(char const* s) { _putcstr(s); }
    void pubsync() { sync(); }

  protected:
    virtual void sync() { _sync(); }
  protected:
    virtual int _sputc(char c) =0;
    virtual mythos::streamsize _sputn(char const* s, mythos::streamsize count) =0;
    virtual void _putcstr(char const* s) =0;
    virtual void _sync() =0;
  };

} // namespace mythos
