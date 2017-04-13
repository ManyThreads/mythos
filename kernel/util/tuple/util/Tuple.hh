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
 * Copyright 2016 Randolf Rotta, BTU Cottbus-Senftenberg
 */
#pragma once

// Mythos supplies an own tuple implementation in order to use the
// same strategy and encoding on host, user, and kernel side
// independently of the respective STL implementations.
//
// see http://mitchnull.blogspot.de/2012/06/c11-tuple-implementation-details-part-1.html
// based on libc++ http://libcxx.llvm.org/ with MIT license

#include <cstddef>
#include <type_traits>
#include <utility>

namespace mythos {
using std::size_t;
 
// forward declaration of our Tuple template
template <typename... Ts_>
class Tuple;
 
namespace detail {
using namespace std;
using std::swap; // needed for primitive types, as ADL won't kick in for them
 
/*** Tuple implementation details come here ***/

// TupleLeaf
 
template<size_t I_,
	 typename ValueType_,
	 bool IsEmpty_ = is_empty<ValueType_>::value && !__is_final(ValueType_)>
class TupleLeaf;
 
template<size_t I, typename ValueType, bool IsEmpty>
inline void swap(TupleLeaf<I, ValueType, IsEmpty>& a,
                 TupleLeaf<I, ValueType, IsEmpty>& b) {
  swap(a.get(), b.get());
}
 
template <size_t I_, typename ValueType_, bool IsEmpty_>
class TupleLeaf 
{
  ValueType_ value_;
     
  TupleLeaf& operator=(const TupleLeaf&) = delete;
public:
  TupleLeaf() : value_() {
    static_assert(!is_reference<ValueType_>::value,
		  "Attempt to default construct a reference element in a tuple");
  }
 
  TupleLeaf(const TupleLeaf& t) : value_(t.get()) {
    static_assert(!is_rvalue_reference<ValueType_>::value,
		  "Can not copy a tuple with rvalue reference member");
  }
 
  template <typename T, 
	    typename = typename enable_if<is_constructible<ValueType_, T>::value>::type>
  explicit TupleLeaf(T&& t) : value_(forward<T>(t)) {
    static_assert(!is_reference<ValueType_>::value ||
		  (is_lvalue_reference<ValueType_>::value &&
		   (is_lvalue_reference<T>::value ||
		    is_same<typename remove_reference<T>::type,
                    reference_wrapper<typename remove_reference<ValueType_>::type>
		    >::value)) ||
		  (is_rvalue_reference<ValueType_>::value &&
		   !is_lvalue_reference<T>::value),
		  "Attempted to construct a reference element in a tuple with an rvalue");
  }
 
  template <typename T>
  explicit TupleLeaf(const TupleLeaf<I_, T>& t) : value_(t.get()) {
  }
 
  template <typename T>
  TupleLeaf& operator=(T&& t) {
    value_ = forward<T>(t);
    return *this;
  }
 
  int swap(TupleLeaf& t) {
    mythos::detail::swap(*this, t);
    return 0;
  }
 
  ValueType_& get() {
    return value_;
  }
  
  const ValueType_& get() const {
    return value_;
  }
};

template <size_t I_, typename ValueType_>
class TupleLeaf<I_, ValueType_, true>
  : private ValueType_ 
{
  TupleLeaf& operator=(const TupleLeaf&) = delete;
public:
  TupleLeaf() {}
  
  TupleLeaf(const TupleLeaf& t) : ValueType_(t.get()) {
    static_assert(!is_rvalue_reference<ValueType_>::value,
		  "Can not copy a tuple with rvalue reference member");
  }
 
  template <typename T, typename = typename enable_if<
			  is_constructible<ValueType_, T>::value>::type>
  explicit TupleLeaf(T&& t) : ValueType_(forward<T>(t)) {}
 
  template <typename T>
  explicit TupleLeaf(const TupleLeaf<I_, T>& t) : ValueType_(t.get()) {}
 
  template <typename T>
  TupleLeaf& operator=(T&& t) {
    ValueType_::operator=(forward<T>(t));
    return *this;
  }
 
  int swap(TupleLeaf& t) {
    mythos::detail::swap(*this, t);
    return 0;
  }
 
  ValueType_& get() {
    return static_cast<ValueType_&>(*this);
  }
 
  const ValueType_& get() const {
    return static_cast<const ValueType_&>(*this);
  }
};

template <size_t... Is_>
struct TupleIndexes 
{
};
 
template <size_t Start_, typename TupleIndexes_, size_t End_>
struct MakeTupleIndexesImpl;
 
template <size_t Start_, size_t... Indexes_, size_t End_>
struct MakeTupleIndexesImpl<Start_, TupleIndexes<Indexes_...>, End_> 
{
  typedef typename MakeTupleIndexesImpl<Start_ + 1,
					TupleIndexes<Indexes_..., Start_>, End_>::type type;
};
 
template <size_t End_, size_t... Indexes_>
struct MakeTupleIndexesImpl<End_, TupleIndexes<Indexes_...>, End_> 
{
  typedef TupleIndexes<Indexes_...> type;
};
 
template <size_t End_, size_t Start_ = 0>
struct MakeTupleIndexes {
  static_assert(Start_ <= End_, "MakeTupleIndexes: invalid parameters");
  typedef typename MakeTupleIndexesImpl<Start_, TupleIndexes<>, End_>::type type;
};
 

template <typename... Ts_>
struct TupleTypes 
{
};

template <typename T_>
struct TupleSize
  : public tuple_size<T_> 
{
};
 
template <typename T_>
struct TupleSize<const T_>
  : public TupleSize<T_> 
{
};
 
template <typename T_>
struct TupleSize<volatile T_>
  : public TupleSize<T_> 
{
};
 
template <typename T_>
struct TupleSize<const volatile T_>
  : public TupleSize<T_> 
{
};
 
template <typename... Ts_>
struct TupleSize<TupleTypes<Ts_...> >
  : public integral_constant<size_t, sizeof...(Ts_)> 
{
};
 
template <typename... Ts_>
struct TupleSize<Tuple<Ts_...> >
  : public integral_constant<size_t, sizeof...(Ts_)>
{
};

template <size_t I_, typename T_>
class TupleElement 
  : public tuple_element<I_, T_> 
{
};
 
template <size_t I_>
class TupleElement<I_, TupleTypes<> >
{
public:
  static_assert(I_ != I_, "tuple_element index out of range");
};
 
template <typename H_, typename... Ts_>
class TupleElement<0, TupleTypes<H_, Ts_...> >
{
public:
  typedef H_ type;
};
 
template <size_t I_, typename H_, typename... Ts_>
class TupleElement<I_, TupleTypes<H_, Ts_...> >
{
public:
  typedef typename TupleElement<I_ - 1, TupleTypes<Ts_...> >::type type;
};
 
template <size_t I_, typename... Ts_>
class TupleElement<I_, Tuple<Ts_...> >
{
public:
  typedef typename TupleElement<I_, TupleTypes<Ts_...> >::type type;
};
 
template <size_t I_, typename... Ts_>
class TupleElement<I_, const Tuple<Ts_...> >
{
public:
  typedef typename add_const<typename TupleElement<I_, TupleTypes<Ts_...> >::type>::type type;
};
 
template <size_t I_, typename... Ts_>
class TupleElement<I_, volatile Tuple<Ts_...> >
{
public:
  typedef typename add_volatile<typename TupleElement<I_, TupleTypes<Ts_...> >::type>::type type;
};
 
template <size_t I_, typename... Ts_>
class TupleElement<I_, const volatile Tuple<Ts_...> >
{
public:
  typedef typename add_cv<typename TupleElement<I_, TupleTypes<Ts_...> >::type>::type type;
};

template <typename TupleTypes_, typename Tuple_, size_t Start_, size_t End_>
struct MakeTupleTypesImpl;
 
template <typename... Types_, typename Tuple_, size_t Start_, size_t End_>
struct MakeTupleTypesImpl<TupleTypes<Types_...>, Tuple_, Start_, End_> 
{
  typedef typename remove_reference<Tuple_>::type TupleType;
  typedef typename MakeTupleTypesImpl<TupleTypes<Types_...,
						 typename conditional<is_lvalue_reference<Tuple_>::value,
								      // append ref if Tuple_ is ref
								      typename TupleElement<Start_, TupleType>::type&,
								      // append non-ref otherwise
								      typename TupleElement<Start_, TupleType>::type>::type>,
				      Tuple_, Start_ + 1, End_>::type  type;
};
 
template <typename... Types_, typename Tuple_, size_t End_>
struct MakeTupleTypesImpl<TupleTypes<Types_...>, Tuple_, End_, End_> 
{
  typedef TupleTypes<Types_...> type;
};
 
template <typename Tuple_,
	  size_t End_ = TupleSize<typename remove_reference<Tuple_>::type>::value,
	  size_t Start_ = 0>
struct MakeTupleTypes 
{
  static_assert(Start_ <= End_, "MakeTupleTypes: invalid parameters");
  typedef typename MakeTupleTypesImpl<TupleTypes<>, Tuple_, Start_, End_>::type type;
};

template <typename... Ts>
void swallow(Ts&&...) {
}
 
template <typename TupleIndexes_, typename... Ts_>
struct TupleImpl;
 
template <size_t... Indexes_, typename... Ts_>
struct TupleImpl<TupleIndexes<Indexes_...>, Ts_...>
  : public TupleLeaf<Indexes_, Ts_>...
{
  template <size_t... FirstIndexes, typename... FirstTypes,
	    size_t... LastIndexes, typename... LastTypes,
	    typename... ValueTypes>
    explicit TupleImpl(
		       TupleIndexes<FirstIndexes...>, TupleTypes<FirstTypes...>,
		       TupleIndexes<LastIndexes...>, TupleTypes<LastTypes...>,
		       ValueTypes&&... values)
    : TupleLeaf<FirstIndexes,
		FirstTypes>(forward<ValueTypes>(values))...
    , TupleLeaf<LastIndexes, LastTypes>()... 
  { }
 
  template <typename OtherTuple>
    TupleImpl(OtherTuple&& t)
    : TupleLeaf<Indexes_, Ts_>(forward<
			       typename TupleElement<Indexes_,
			       typename MakeTupleTypes<OtherTuple>::type>::type
			       >(get<Indexes_>(t)))... 
  { }
 
  template <typename OtherTuple>
    TupleImpl& operator=(OtherTuple&& t) {
    swallow(TupleLeaf<Indexes_, Ts_>::operator=(forward<typename TupleElement<Indexes_,	typename MakeTupleTypes<OtherTuple>::type>::type>(get<Indexes_>(t)))...);
    return *this;
  }
 
  TupleImpl& operator=(const TupleImpl& t) {
    swallow(TupleLeaf<Indexes_, Ts_>::operator=(static_cast<const TupleLeaf<Indexes_, Ts_>&>(t).get())...);
    return *this;
  }
 
  void swap(TupleImpl& t) {
    swallow(TupleLeaf<Indexes_, Ts_>::swap(static_cast<TupleLeaf<Indexes_, Ts_>&>(t))...);
  }
};

template <typename T_>
struct TupleLike : public false_type {};
 
template <typename T_>
struct TupleLike<const T_> : public TupleLike<T_> {};
 
template <typename T_>
struct TupleLike<volatile T_> : public TupleLike<T_> {};
 
template <typename T_>
struct TupleLike<const volatile T_> : public TupleLike<T_> {};
 
template <typename... Ts_>
struct TupleLike<Tuple<Ts_...> > : public true_type {};
 
template <typename... Ts_>
struct TupleLike<TupleTypes<Ts_...> > : public true_type {};

template <typename... Ts_>
struct TupleLike<std::tuple<Ts_...> > : public true_type {};
 
template <typename First_, typename Second_>
struct TupleLike<std::pair<First_, Second_> > : public true_type {};
 
// template <typename T_, size_t N_>
// struct TupleLike<std::array<T_, N_> > : public true_type {};

template <bool IsSameSize_, typename From_, typename To_>
struct TupleConvertibleImpl
        :  public false_type {
};
 
template <typename From0_, typename... FromRest_,
	  typename To0_, typename... ToRest_>
struct TupleConvertibleImpl<true, TupleTypes<From0_, FromRest_...>,
            TupleTypes<To0_, ToRest_...> >
  : public integral_constant<bool,
			     is_convertible<From0_, To0_>::value &&
			     TupleConvertibleImpl<true, TupleTypes<FromRest_...>,
						  TupleTypes<ToRest_...> >::value> 
{
};
 
template <>
struct TupleConvertibleImpl<true, TupleTypes<>, TupleTypes<> >  : public true_type {};
 
template <typename From_, typename To_,
	  bool = TupleLike<typename remove_reference<From_>::type>::value,
	  bool = TupleLike<typename remove_reference<To_>::type>::value>
struct TupleConvertible : public false_type {};
 
template<typename From_, typename To_>
struct TupleConvertible<From_, To_, true, true>
  : public TupleConvertibleImpl<
  TupleSize<typename remove_reference<From_>::type>::value == TupleSize<typename remove_reference<To_>::type>::value,
  typename MakeTupleTypes<From_>::type,
  typename MakeTupleTypes<To_>::type> 
{
};

template <bool IsSameSize_, typename Target_, typename From_>
struct TupleAssignableImpl
        :  public false_type {
};
 
template <typename Target0_, typename... TargetRest_,
	  typename From0_, typename... FromRest_>
struct TupleAssignableImpl<true, TupleTypes<Target0_, TargetRest_...>,
			   TupleTypes<From0_, FromRest_...> >
  : public integral_constant<bool,
			     is_assignable<Target0_, From0_>::value &&
			     TupleAssignableImpl<true, TupleTypes<TargetRest_...>,
						 TupleTypes<FromRest_...>>::value>
{
};
 
template <>
struct TupleAssignableImpl<true, TupleTypes<>, TupleTypes<> > : public true_type {};
 
template <typename Target_, typename From_,
	  bool = TupleLike<typename remove_reference<Target_>::type>::value,
	  bool = TupleLike<typename remove_reference<From_>::type>::value>
struct TupleAssignable : public false_type {};
 
template<typename Target_, typename From_>
struct TupleAssignable<Target_, From_, true, true>
  : public TupleAssignableImpl<
  TupleSize<typename remove_reference<Target_>::type>::value
  == TupleSize<typename remove_reference<From_>::type>::value,
  typename MakeTupleTypes<Target_>::type,
  typename MakeTupleTypes<From_>::type> 
{
};


template <size_t I_>
struct TupleEqual 
{
  template <typename Tuple1_, typename Tuple2_>
  bool operator()(const Tuple1_& t1, const Tuple2_& t2){
    return TupleEqual<I_ - 1>()(t1, t2) && get<I_ - 1>(t1) == get<I_ - 1>(t2);
  }
};
 
template<>
struct TupleEqual<0> 
{
  template <typename Tuple1_, typename Tuple2_>
  bool operator()(const Tuple1_&, const Tuple2_&) { return true; }
};
 
template <size_t I_>
struct TupleLess 
{
  template <typename Tuple1_, typename Tuple2_>
  bool operator()(const Tuple1_& t1, const Tuple2_& t2) {
    return TupleLess<I_ - 1>()(t1, t2) ||  (!TupleLess<I_ - 1>()(t2, t1) && get<I_ - 1>(t1) < get<I_ - 1>(t2));
  }
};
 
template<>
struct TupleLess<0> 
{
  template <typename Tuple1_, typename Tuple2_>
  bool operator()(const Tuple1_&, const Tuple2_&) { return false; }
};

} // end namespace detail
 
/*** user-visible Tuple and related functions come here ***/

// tuple_size
 
template <typename Tuple_>
class tuple_size : public detail::TupleSize<Tuple_> {};
 
// tuple_element
 
template <size_t I_, typename Tuple_>
class tuple_element : public detail::TupleElement<I_, Tuple_> {};
 
// Tuple
 
template <typename... Ts_>
class Tuple  {
  typedef detail::TupleImpl<typename detail::MakeTupleIndexes<sizeof...(Ts_)>::type, Ts_...> Impl;
  Impl impl_;
 
  template <size_t I, typename... Ts>
  friend typename tuple_element<I, Tuple<Ts...>>::type& get(Tuple<Ts...>& t);
 
  template <size_t I, typename... Ts>
  friend const typename tuple_element<I, Tuple<Ts...>>::type& get(const Tuple<Ts...>& t);
 
  template <size_t I, typename... Ts>
  friend typename tuple_element<I, Tuple<Ts...>>::type&& get(Tuple<Ts...>&& t);
 
public:
  explicit Tuple(const Ts_&... t)
    : impl_(typename detail::MakeTupleIndexes<sizeof...(Ts_)>::type(),
	    typename detail::MakeTupleTypes<Tuple, sizeof...(Ts_)>::type(),
	    typename detail::MakeTupleIndexes<0>::type(),
	    typename detail::MakeTupleTypes<Tuple, 0>::type(),
	    t...) 
  { }
 
  template <typename... Us, typename std::enable_if<
			      sizeof...(Us) <= sizeof...(Ts_) &&
    detail::TupleConvertible<
    typename detail::MakeTupleTypes<Tuple<Us...>>::type,
				typename detail::MakeTupleTypes<Tuple,
								sizeof...(Us) < sizeof...(Ts_)
										? sizeof...(Us)
										: sizeof...(Ts_)>::type>::value,
				bool>::type = false>
    explicit Tuple(Us&&... u)
    : impl_(typename detail::MakeTupleIndexes<sizeof...(Us)>::type(),
	    typename detail::MakeTupleTypes<Tuple, sizeof...(Us)>::type(),
	    typename detail::MakeTupleIndexes<sizeof...(Ts_), sizeof...(Us)>::type(),
	    typename detail::MakeTupleTypes<Tuple, sizeof...(Ts_), sizeof...(Us)>::type(),
	    std::forward<Us>(u)...) 
  { }
                     
     
    template <typename OtherTuple, typename std::enable_if<
				     detail::TupleConvertible<OtherTuple, Tuple>::value,
				     bool>::type = false>
    Tuple(OtherTuple&& t) : impl_(std::forward<OtherTuple>(t)) { }
 
  template <typename OtherTuple, typename std::enable_if<
				   detail::TupleAssignable<Tuple, OtherTuple>::value,
				   bool>::type = false>
  Tuple& operator=(OtherTuple&& t) {
    impl_.operator=(std::forward<OtherTuple>(t));
    return *this;
  }
 
  void swap(Tuple& t) {
    impl_.swap(t.impl_);
  }
};



 
// Empty Tuple
 
template <>
class Tuple<> {
public:
    Tuple() {
    }
 
    template <typename OtherTuple, typename std::enable_if<
            detail::TupleConvertible<OtherTuple, Tuple>::value,
            bool>::type = false>
    Tuple(OtherTuple&&) {
    }
 
    template <typename OtherTuple, typename std::enable_if<
            detail::TupleAssignable<Tuple, OtherTuple>::value,
            bool>::type = false>
    Tuple& operator=(OtherTuple&&) {
        return *this;
    }
 
    void swap(Tuple&) {
    }
};
 
// get
 
template <size_t I, typename... Ts>
inline
typename tuple_element<I, Tuple<Ts...>>::type&
get(Tuple<Ts...>& t) {
    typedef typename tuple_element<I, Tuple<Ts...>>::type Type;
    return static_cast<detail::TupleLeaf<I, Type>&>(t.impl_).get();
}
 
template <size_t I, typename... Ts>
inline
const typename tuple_element<I, Tuple<Ts...>>::type&
get(const Tuple<Ts...>& t) {
    typedef typename tuple_element<I, Tuple<Ts...>>::type Type;
    return static_cast<const detail::TupleLeaf<I, Type>&>(t.impl_).get();
}
 
template <size_t I, typename... Ts>
inline
typename tuple_element<I, Tuple<Ts...>>::type&&
get(Tuple<Ts...>&& t) {
    typedef typename tuple_element<I, Tuple<Ts...>>::type Type;
    return static_cast<Type&&>(static_cast<detail::TupleLeaf<I, Type>&&>(t.impl_).get());
}
 
// swap
 
template <typename... Ts>
inline
void swap(Tuple<Ts...>& a, Tuple<Ts...>& b) {
    a.swap(b);
}
 
// relational operators
 
template <typename... T1s_, typename... T2s_>
inline bool operator==(const Tuple<T1s_...>& t1, const Tuple<T2s_...>& t2) {
    return detail::TupleEqual<sizeof...(T1s_)>()(t1, t2);
}
 
template <typename... T1s_, typename... T2s_>
inline bool operator!=(const Tuple<T1s_...>& t1, const Tuple<T2s_...>& t2) {
    return !(t1 == t2);
}
 
template <typename... T1s_, typename... T2s_>
inline bool operator<(const Tuple<T1s_...>& t1, const Tuple<T2s_...>& t2) {
    return detail::TupleLess<sizeof...(T1s_)>()(t1, t2);
}
 
template <typename... T1s_, typename... T2s_>
inline bool operator>(const Tuple<T1s_...>& t1, const Tuple<T2s_...>& t2) {
    return t2 < t1;
}
 
template <typename... T1s_, typename... T2s_>
inline bool operator<=(const Tuple<T1s_...>& t1, const Tuple<T2s_...>& t2) {
    return !(t2 < t1);
}
 
template <typename... T1s_, typename... T2s_>
inline bool operator>=(const Tuple<T1s_...>& t1, const Tuple<T2s_...>& t2) {
    return !(t1 < t2);
}
 
} // end namespace mythos
 
namespace std {
 
/*** specializations of tuple_size, tuple_element and get for our Tuple come here ***/

// tuple_size
 
  template <typename... Ts_>
  class tuple_size<mythos::Tuple<Ts_...> >
    : public mythos::tuple_size<mythos::Tuple<Ts_...> > {};
 
  template <typename... Ts_>
  class tuple_size<const mythos::Tuple<Ts_...> >
    : public mythos::tuple_size<const mythos::Tuple<Ts_...> > {};
 
  template <typename... Ts_>
  class tuple_size<volatile mythos::Tuple<Ts_...> >
    : public mythos::tuple_size<volatile mythos::Tuple<Ts_...> > {};
 
  template <typename... Ts_>
  class tuple_size<const volatile mythos::Tuple<Ts_...> >
    : public mythos::tuple_size<const volatile mythos::Tuple<Ts_...> > {};
 
  // tuple_element
 
  template <size_t I_, typename... Ts_>
  class tuple_element<I_, mythos::Tuple<Ts_...> >
    : public mythos::tuple_element<I_, mythos::Tuple<Ts_...> > {};
 
  template <size_t I_, typename... Ts_>
  class tuple_element<I_, const mythos::Tuple<Ts_...> >
    : public mythos::tuple_element<I_, const mythos::Tuple<Ts_...> > {};
 
  template <size_t I_, typename... Ts_>
  class tuple_element<I_, volatile mythos::Tuple<Ts_...> >
    : public mythos::tuple_element<I_, volatile mythos::Tuple<Ts_...> > {};
 
  template <size_t I_, typename... Ts_>
  class tuple_element<I_, const volatile mythos::Tuple<Ts_...>>
    : public mythos::tuple_element<I_, const volatile mythos::Tuple<Ts_...> > {};
 
  using mythos::get;

} // end namespace std
