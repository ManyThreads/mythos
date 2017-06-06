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

#include "mythos/Error.hh"
#include "util/optional.hh"
#include "async/Tasklet.hh"

namespace mythos {
  namespace async {

    template<class T>
    class IResult
    {
    public:
      virtual ~IResult() {}
      virtual void response(Tasklet*, optional<T>) = 0;
      inline void response(Tasklet* t, Error err) { response(t, optional<T>(err)); }
    };

    template<>
    class IResult<void>
    {
    public:
      virtual ~IResult() {}
      void response(Tasklet* t) { response(t, Error::SUCCESS); }
      virtual void response(Tasklet*, optional<void>) = 0;
      inline void response(Tasklet* t, Error err) { response(t, optional<void>(err)); }
    };

    // use as: MSink<MyClass, ResType, &MyClass::fooResponse> fooSink{this};
    // with void MyClass::fooResponse(Tasklet*, optional<ResType>) { ... }
    template<class O, class T, void(O::*M)(Tasklet*, optional<T>)>
    class MSink
      : public IResult<T>
    {
    public:
      MSink(O* obj) : obj(obj) {}
      virtual ~MSink() {}
      void response(Tasklet* t, optional<T> v) override { (obj->*M)(t, v); }
    protected:
      O* obj;
    };

    // untested and imperfect
    template<class O, class T>
    class Sink
      : protected IResult<T>
    {
    public:
      Sink(O* obj) : obj(obj), tsk(nullptr) {}
      virtual ~Sink() {}

      Error state() const WARN_UNUSED { return val.state(); }
      bool operator! () const WARN_UNUSED { return val.isError(); }
      explicit operator bool() const WARN_UNUSED { return val.isSuccess(); }

      optional<T> const& operator-> () const { return val; }
      optional<T>& operator-> () { return val; }
      T const& operator*() const { return *val; }
      T& operator* () { return *val; }

      Tasklet* tasklet() { return tsk; }

      IResult<T>* ptr() { tsk = nullptr; val = Error::UNSET; return IResult<T>(this); }

    private:
      virtual void response(Tasklet* t, optional<T> v) {
        ASSERT(v);
        val = v;
        obj->receivedResponse(t, this);
      }

    protected:
      O* obj;
      Tasklet* tsk;
      optional<T> val;
    };

  } // namespace async

  using async::IResult;

} // namespace mythos
