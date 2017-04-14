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
 * Copyright 2016 Randolf Rotta, Robert Kuban, BTU Cottbus-Senftenberg
 */
#pragma once

#include "util/Tuple.hh"
#include "util/integer_sequence.hh"
#include <tuple>

namespace mythos {

  namespace detail {
    template<int ...S, class Functor, class... Args>
    void applyTuple(Functor func, std::tuple<Args...> tpl, seq<S...>)
    {
      func(std::get<S>(tpl)...);
    }

    template<int ...S, class Functor, class... Args>
    void applyTuple(Functor func, mythos::Tuple<Args...> tpl, seq<S...>)
    {
      func(std::get<S>(tpl)...);
    }

    
  } // namespace detail    
    
  /** Call a functor with the tuple contents as arguments.
   * based on: http://stackoverflow.com/questions/7858817/unpacking-a-tuple-to-call-a-matching-function-pointer
   */
  template<class Functor, class... Args>
  void apply(Functor func, std::tuple<Args...> tpl)
  {
    detail::applyTuple(func, tpl, typename gens<sizeof...(Args)>::type());
  }

  /** Call a functor with the tuple contents as arguments.  */
  template<class Functor, class... Args>
  void apply(Functor func, mythos::Tuple<Args...> tpl)
  {
    detail::applyTuple(func, tpl, typename gens<sizeof...(Args)>::type());
  }

} // namespace mythos
