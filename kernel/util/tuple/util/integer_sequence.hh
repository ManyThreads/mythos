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

namespace mythos {

  // copy from libcxx include/utility

  template<class _Tp, _Tp... _Ip>
  struct integer_sequence
  {
    typedef _Tp value_type;
    static constexpr size_t size() noexcept { return sizeof...(_Ip); }
  };

  template<size_t... _Ip>
  using index_sequence = integer_sequence<size_t, _Ip...>;

  namespace __detail {

    template<typename _Tp, size_t ..._Extra> struct __repeat;
    template<typename _Tp, _Tp ..._Np, size_t ..._Extra> struct __repeat<integer_sequence<_Tp, _Np...>, _Extra...> {
      typedef integer_sequence<_Tp, _Np..., sizeof...(_Np) + _Np...,
	2 * sizeof...(_Np) + _Np...,
	3 * sizeof...(_Np) + _Np...,
	4 * sizeof...(_Np) + _Np...,
	5 * sizeof...(_Np) + _Np...,
	6 * sizeof...(_Np) + _Np...,
	7 * sizeof...(_Np) + _Np...,
	_Extra...> type;
    };

    template<size_t _Np> struct __parity;
    template<size_t _Np> struct __make : __parity<_Np % 8>::template __pmake<_Np> {};

    template<> struct __make<0> { typedef integer_sequence<size_t> type; };
    template<> struct __make<1> { typedef integer_sequence<size_t, 0> type; };
    template<> struct __make<2> { typedef integer_sequence<size_t, 0, 1> type; };
    template<> struct __make<3> { typedef integer_sequence<size_t, 0, 1, 2> type; };
    template<> struct __make<4> { typedef integer_sequence<size_t, 0, 1, 2, 3> type; };
    template<> struct __make<5> { typedef integer_sequence<size_t, 0, 1, 2, 3, 4> type; };
    template<> struct __make<6> { typedef integer_sequence<size_t, 0, 1, 2, 3, 4, 5> type; };
    template<> struct __make<7> { typedef integer_sequence<size_t, 0, 1, 2, 3, 4, 5, 6> type; };
    
    template<> struct __parity<0> { template<size_t _Np> struct __pmake : __repeat<typename __make<_Np / 8>::type> {}; };
    template<> struct __parity<1> { template<size_t _Np> struct __pmake : __repeat<typename __make<_Np / 8>::type, _Np - 1> {}; };
    template<> struct __parity<2> { template<size_t _Np> struct __pmake : __repeat<typename __make<_Np / 8>::type, _Np - 2, _Np - 1> {}; };
    template<> struct __parity<3> { template<size_t _Np> struct __pmake : __repeat<typename __make<_Np / 8>::type, _Np - 3, _Np - 2, _Np - 1> {}; };
    template<> struct __parity<4> { template<size_t _Np> struct __pmake : __repeat<typename __make<_Np / 8>::type, _Np - 4, _Np - 3, _Np - 2, _Np - 1> {}; };
    template<> struct __parity<5> { template<size_t _Np> struct __pmake : __repeat<typename __make<_Np / 8>::type, _Np - 5, _Np - 4, _Np - 3, _Np - 2, _Np - 1> {}; };
    template<> struct __parity<6> { template<size_t _Np> struct __pmake : __repeat<typename __make<_Np / 8>::type, _Np - 6, _Np - 5, _Np - 4, _Np - 3, _Np - 2, _Np - 1> {}; };
    template<> struct __parity<7> { template<size_t _Np> struct __pmake : __repeat<typename __make<_Np / 8>::type, _Np - 7, _Np - 6, _Np - 5, _Np - 4, _Np - 3, _Np - 2, _Np - 1> {}; };

    template<typename _Tp, typename _Up> struct __convert {
      template<typename> struct __result;
      template<_Tp ..._Np> struct __result<integer_sequence<_Tp, _Np...> > { typedef integer_sequence<_Up, _Np...> type; };
    };
    template<typename _Tp> struct __convert<_Tp, _Tp> { template<typename _Up> struct __result { typedef _Up type; }; };
    
  } // namespace detail

  template<typename _Tp, _Tp _Np> using __make_integer_sequence_unchecked =
    typename __detail::__convert<size_t, _Tp>::template __result<typename __detail::__make<_Np>::type>::type;

  template <class _Tp, _Tp _Ep>
  struct __make_integer_sequence
  {
    static_assert(0 <= _Ep, "std::make_integer_sequence input shall not be negative");
    typedef __make_integer_sequence_unchecked<_Tp, _Ep> type;
  };

  template<class _Tp, _Tp _Np>
  using make_integer_sequence = typename __make_integer_sequence<_Tp, _Np>::type;

  template<size_t _Np>
  using make_index_sequence = make_integer_sequence<size_t, _Np>;

  template<class... _Tp>
  using index_sequence_for = make_index_sequence<sizeof...(_Tp)>;
  
} //namespace mythos
