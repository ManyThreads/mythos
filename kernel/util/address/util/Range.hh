/* -*- mode:C++; c-file-style:"bsd"; -*- */
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
 * Copyright 2013 Randolf Rotta, Maik Krüger, and contributors, BTU Cottbus-Senftenberg
 */
#pragma once

#include <cstddef>
#include "util/ostream.hh"

namespace mythos {

/** interval [start, end). */
template<typename T>
class Range
{
public:
  Range() : start(0), end(0) {}
  Range(T start, T end) : start(start), end(end) {}

  static Range bySize(T start, size_t size) {
    return Range(start, T(uintptr_t(start)+size));
  }

  T getStart() const { return start; }
  T getEnd() const { return end; }
  T getSize() const { return end-start; }

  void setStart(T s) { start = s; }
  void setEnd(T e) {  end = e; }

  bool isEmpty() const { return getSize() == 0; }

  Range cut(Range const& rhs) const {
    T s = getStart() > rhs.getStart() ? getStart() : rhs.getStart();
    T e = getEnd() < rhs.getEnd() ? getEnd() : rhs.getEnd();
    return Range(s, e>s ? e : s);
  }

  bool contains(const T& point) const {
    return (getStart() <= point) && (point < getEnd());
  }

  bool contains(const Range& other) const {
    return (getStart() <= other.getStart())
      && (other.getEnd() <= getEnd());
  }

  bool overlaps(Range const& rhs) const {
    return (getStart() <= rhs.getStart() && getEnd() > rhs.getStart()) ||
      (rhs.getStart() <= getStart() && rhs.getEnd() > getStart());
  }

  bool operator==(Range const& rhs) const {
    return (getStart() == rhs.getStart()) && (getEnd()==rhs.getEnd());
  }

  bool operator!=(Range const& rhs) const { return !operator==(rhs); }

protected:
  T start;
  T end;
};

} // namespace mythos

namespace mlog {

using mythos::ostream_base;

template<class S, class T>
ostream_base<S>& operator<<(ostream_base<S>& out, const mythos::Range<T> r)
{
  out << '<' << r.getStart() << ".." << r.getEnd() << ':' << r.getSize() << '>';
  return out;
}

} // namespace mlog

