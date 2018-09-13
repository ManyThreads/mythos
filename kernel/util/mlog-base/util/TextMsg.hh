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
#include "util/FixedStreamBuf.hh"
#include "util/TextMsgHelpers.hh"
#include "util/mstring.hh"
#include <utility> // for std::forward

namespace mlog {

  template<size_t MAXSIZE>
  class TextMsg {
  public:
    typedef mythos::FixedStreamBuf<MAXSIZE> str_t;
    typedef mythos::ostream_base<str_t> ostr_t;
    
    template<typename... ARGS>
    TextMsg(char const* prefix, char const* subsys, ARGS&&... args) {
      ostr_t stream(&buf);
      stream << prefix << subsys;
      add(stream, std::forward<ARGS>(args)...);
      end(stream);
    }
    
    char const* getData() const { return buf.c_str(); }
    size_t getSize() const { return buf.size(); }

  protected: // for derived message classes
    template<typename A, typename... ARGS>
    void add(ostr_t& stream, A&& a, ARGS&&... args) {
      stream << " " << a; add(stream, std::forward<ARGS>(args)...);
    }

    template<typename A>
    void add(ostr_t& stream, A&& a) { stream << " " << a; }
    
    void end(ostr_t& stream) {
      char const str[] = "\x1b[0m";
      char const strover[] = "...\x1b[0m";
      stream << str;
      if (buf.size()<MAXSIZE-sizeof(str)+1)
	mythos::memcpy(this->buf.c_str() + this->buf.size()-sizeof(str)+1, str, sizeof(str)-1);
      else
	mythos::memcpy(this->buf.c_str() + this->buf.size()-sizeof(strover)+1, strover, sizeof(strover)-1);
    }
    
    str_t buf;
  };

  struct TextDetail
    : public TextMsg<400>
  {
    constexpr static size_t VERBOSITY = 0;
    template<typename... ARGS>
    TextDetail(char const* subsys, ARGS&&... args)
      : TextMsg<400>("\x1b[0;34m", subsys, std::forward<ARGS>(args)...) {}
  };
  
  struct TextInfo
    : public TextMsg<400>
  {
    constexpr static size_t VERBOSITY = 1;
    template<typename... ARGS>
    TextInfo(char const* subsys, ARGS&&... args)
      : TextMsg<400>("\x1b[0;97m", subsys, std::forward<ARGS>(args)...) {}
  };

  struct TextWarning
    : public TextMsg<400>
  {
    constexpr static size_t VERBOSITY = 2;
    template<typename... ARGS>
    TextWarning(char const* subsys, ARGS&&... args)
      : TextMsg<400>("\x1b[0;33;1m", subsys, std::forward<ARGS>(args)...) {}
  };

  struct TextError
    : public TextMsg<400>
  {
    constexpr static size_t VERBOSITY = 3;
    template<typename... ARGS>
    TextError(char const* subsys, ARGS&&... args)
      : TextMsg<400>("\x1b[0;31;1m", subsys, std::forward<ARGS>(args)...) {}
  };

  struct TextFailure
    : public TextMsg<400>
  {
    constexpr static size_t VERBOSITY = 3;
    template<typename... ARGS>
    TextFailure(char const* subsys, ARGS&&... args)
      : TextMsg<400>("\x1b[0;91;1m", subsys, std::forward<ARGS>(args)...) {}
  };

  struct TextSuccess
    : public TextMsg<400>
  {
    constexpr static size_t VERBOSITY = 3;
    template<typename... ARGS>
    TextSuccess(char const* subsys, ARGS&&... args)
      : TextMsg<400>("\x1b[0;92;1m", subsys, std::forward<ARGS>(args)...) {}
  };

  template<size_t MAXSIZE>
  class CsvTextMsg {
  public:
    typedef mythos::FixedStreamBuf<MAXSIZE> str_t;
    typedef mythos::ostream_base<str_t> ostr_t;
    
    template<typename... ARGS>
    CsvTextMsg(char const* subsys, ARGS&&... args) {
      ostr_t stream(&buf);
      stream << subsys;
      add(stream, std::forward<ARGS>(args)...);
      end(stream);
    }
    
    char const* getData() const { return buf.c_str(); }
    size_t getSize() const { return buf.size(); }

  protected: // for derived message classes
    template<typename A, typename... ARGS>
    void add(ostr_t& stream, A&& a, ARGS&&... args) {
      stream << ", " << a; add(stream, std::forward<ARGS>(args)...);
    }

    template<typename A>
    void add(ostr_t& stream, A&& a) { stream << ", " << a; }
    
    void end(ostr_t& stream) {
      char const str[] = "";
      char const strover[] = "...";
      stream << str;
      if (buf.size()<MAXSIZE-sizeof(str)+1)
	mythos::memcpy(this->buf.c_str() + this->buf.size()-sizeof(str)+1, str, sizeof(str)-1);
      else
	mythos::memcpy(this->buf.c_str() + this->buf.size()-sizeof(strover)+1, strover, sizeof(strover)-1);
    }
    
    str_t buf;
  };

  struct TextCSV
    : public CsvTextMsg<512>
  {
    constexpr static size_t VERBOSITY = 3;
    template<typename... ARGS>
    TextCSV(char const* table, ARGS&&... args)
      : CsvTextMsg<512>(table, std::forward<ARGS>(args)...) {}
  };
  
} // namespace mlog
