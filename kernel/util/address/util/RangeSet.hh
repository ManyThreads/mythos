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

#include "util/Range.hh"
#include "util/VectorMax.hh"

namespace mythos {

template<typename T, size_t M>
class RangeSet
  : public VectorMax<Range<T>,M>
{
public:
  RangeSet() {}

  void add(T start, T end) { this->add(Range<T>(start,end)); }
  void add(Range<T> const& rhs) {
    // @TODO check for overlap with existing intervals
    this->push_back(rhs);
  }

  void substract(T start, T end) { this->substract(Range<T>(start,end)); }
  void substract(Range<T> const& rhs) {
    if (rhs.isEmpty()) return;
    for (size_t i=0; i<this->size(); i++) {
      Range<T>& lhs = this->get(i); 

      // should be 6 cases of overlapping intervals
      if (rhs.getStart() <= lhs.getStart()) {
	// rS <= lS
	if (lhs.getEnd() <= rhs.getEnd()) {
	  // rS <= lS < lE <= rE ==> remove lhs
	  lhs = this->get(this->size()-1);
	  this->pop_back();	
	  i--;
	} else if (lhs.getStart() < rhs.getEnd()) { 
	  // rS <= lS < rE < lE ==> remove left part of lhs
	  lhs.setStart(rhs.getEnd());
	} // else rE <= lS and nothing to do
      } else if (rhs.getStart() < lhs.getEnd()) { 
	// lS < rS < lE, otherwise nothing to do
	if (lhs.getEnd() <= rhs.getEnd()) {
	  // lS < rS < lE <= rE ==> remove right part of lhs
	  lhs.setEnd(rhs.getStart());
	} else {
	  // lS < rS < rE < lE ==> make a hole in lhs
	  this->push_back(Range<T>(rhs.getEnd(),lhs.getEnd()));
	  lhs.setEnd(rhs.getStart());
	}
      } // else lE <= rS and nothing to do
    }
  }

};

} // namespace mythos
