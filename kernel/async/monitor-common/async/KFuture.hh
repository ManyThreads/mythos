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
 * Copyright 2016 Randolf Rotta, Robert Kuban, Maik Krüger, and contributors, BTU Cottbus-Senftenberg
 */
#pragma once

#include "async/IResult.hh"
#include "async/Place.hh"

namespace mythos {
  namespace async {

    /** Most evil future. Do not use inside asynchronous calls! Only
     * valid on the kernels main thread during system's startup. */
    template<class... T>
    class KFuture
      : public IResult<T...>
    {
    public:
      virtual ~KFuture() {}
      virtual void response(Tasklet*, optional<T...> v) { val = v; }
      optional<T...> const& wait();
      void reset() { val = optional<T...>(Error::UNSET); }
    protected:
      optional<T...> val;
    };

    template<class... T>
    optional<T...> const& KFuture<T...>::wait() {
      while (val.state() == Error::UNSET) {
	mythos::async::getLocalPlace().processTasks();
      }
      return val;
    }
    
  } // namespace async
} // namespace mythos
