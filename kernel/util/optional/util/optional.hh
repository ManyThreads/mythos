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

#include <cstdint>
#include <new>
#include <utility>
#include <type_traits>
#include "util/assert.hh"
#include "util/compiler.hh"
#include "mythos/Error.hh"
#include "util/ostream.hh"

namespace mythos {

class OptionalBase {
public:
  OptionalBase(Error merror) : merror(merror) {}
  Error state() const { setChecked(); return merror; }
  bool isUnset() const { return merror == Error::UNSET; }
  bool isSuccess() const { return state() == Error::SUCCESS; }
  bool isError() const { return !isSuccess(); }
  bool operator! () const { return isError(); }
  explicit operator bool() const { return isSuccess(); }

protected:
  Error merror;

#ifdef NDEBUG
  void setChecked() const {};
  bool isChecked() const { return true; }
  void _check() const {}
#else
  mutable bool mchecked = false;
  void setChecked() const { mchecked = true; };
  bool isChecked() const { return mchecked; }
  void _check() const {
    OOPS_MSG(isChecked(), "Unchecked use of optional value.");
    ASSERT_MSG(merror == Error::SUCCESS, "no optional value");
  }
#endif
};

template<typename T> struct optional;
template<class T> struct optional<T*>;
template<> struct optional<void>;

template<typename T>
struct optional
  : public OptionalBase
{
public:
  static_assert(!std::is_reference<T>::value, "references not supported");
  static_assert(!std::is_pointer<T>::value, "pointers not supported");
  typedef T value_t;
  optional() : OptionalBase(Error::UNSET) {}

  explicit optional(Error merror) : OptionalBase(merror) {}
  optional(T const& w) : OptionalBase(Error::SUCCESS), value(w) {}
  optional(optional const& rhs) : OptionalBase(rhs.merror), value(rhs.value) {}

  optional& operator= (optional const& rhs) { merror=rhs.merror; value=rhs.value; return *this; }
  //optional& operator= (optional&& rhs) { merror=rhs.merror; value=rhs.value; return *this; }
  optional& operator= (T const& w) { merror = Error::SUCCESS; value=w; return *this; }
  T const* operator-> () const { _check(); return &value; }
  T const& operator* () const { _check(); return value; }

protected:
  T value;
};

template<>
struct optional<void>
  : public OptionalBase
{
public:
  optional() : OptionalBase(Error::UNSET) {}
  explicit optional(Error merror) : OptionalBase(merror) {}
  template<class T> optional(optional<T> const& rhs) : OptionalBase(rhs.state()) {}
  template<class T> operator optional<T>() {
    ASSERT(merror!=Error::SUCCESS && merror!=Error::INHIBIT);
    return optional<T>(state());
  }
};

template<typename T>
struct optional<T*>
  : public OptionalBase
{
public:
  typedef T* value_t;
  optional() : OptionalBase(Error::UNSET) {}
  explicit optional(Error merror) : OptionalBase(merror) {}
  optional(T* w) : OptionalBase(Error::SUCCESS), value(w) {}
  optional(optional const& rhs) : OptionalBase(rhs.merror), value(rhs.value) {}
  optional& operator= (optional const& rhs) { merror=rhs.merror; value=rhs.value; return *this; }
  //optional& operator= (optional&& rhs) { merror=rhs.merror; value=rhs.value; return *this; }
  optional& operator= (T* w) { merror = Error::SUCCESS; value=w; return *this; }
  T* operator-> () const { _check(); return value; }
  T* operator* () const { _check(); return value; }

protected:
  T* value;
};

  template<class S>
  ostream_base<S>& operator<< (ostream_base<S>& out, Error e) {
    out << "E" << static_cast<int>(e); return out;
  }

  template<class S, class T>
  ostream_base<S>& operator<< (ostream_base<S>& out, optional<T> const& e) {
    if (e) out << "O-" << *e;
    else out << "OE" << static_cast<int>(e.state());
    return out;
  }

  inline optional<void> boxError(const Error& err) { return optional<void>(err); }

  template<class T>
  inline optional<T> boxError(const optional<T>& opt) { return opt; }

} // namespace mythos
