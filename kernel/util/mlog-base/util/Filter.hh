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

#include <utility> // for std::forward

namespace mlog {

  struct FilterDyn
  {
  public:
    size_t verbosity;
    FilterDyn(size_t verbosity=0) : verbosity(verbosity) {}
    void setVerbosity(size_t verbosity) { this->verbosity = verbosity; }
    bool isActive(size_t v) const { return v >= verbosity; }
  };

  struct FilterAny
  {
    constexpr bool isActive(size_t) const { return true; }
  };

  template<size_t V>
  struct FilterStatic
  {
    constexpr bool isActive(size_t v) const { return v >= V; }
  };

  typedef FilterStatic<0> FilterDetail;
  typedef FilterStatic<1> FilterInfo;
  typedef FilterStatic<2> FilterWarning;
  typedef FilterStatic<3> FilterError;


  
  class FilterTraceDiscard
  {
  public:
    template<class MSG, typename... ARGS>
    bool accept(ARGS&&...) const { 
      return false;
    }
  };

  class FilterTrace
  {
  public:
    template<class MSG, typename... ARGS>
    bool accept(ARGS&&...) const { 
      #ifdef TRACE
        return true;
      #else
        return false;
      #endif
    }
  };

  class FilterTraceDyn
  {
  public:
    FilterTraceDyn(bool flag=true) : flag(flag) {}
    void setFlag(bool flag) { this->flag = flag; }


    template<class MSG, typename... ARGS>
    bool accept(ARGS&&...) const { return flag; }

    protected:
    bool flag;
  };
  
} // namespace mlog
