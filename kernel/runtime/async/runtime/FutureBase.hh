/* -*- mode:C++; indent-tabs-mode:nil; -*- */
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
 * Copyright 2016 Randolf Rotta, Robert Kuban, and contributors, BTU Cottbus-Senftenberg
 */
#pragma once

#include "mythos/Error.hh"
#include "util/assert.hh"
#include "runtime/Tasklet.hh"

namespace mythos {

  // based on hewitt paper: a future is (process, value, waiters): process is a pointer to the ongoing computation (or the async task will produces the value?); value will contain the result once available. it's valid if process == nullptr; waiters is a list of processes to wakeup (a list of tasks to execute) once the value is available. The value also contains an error value which is used when the computation failed.
  // a promise represents a computation that will produce the value later. was initially intended as means for lazy evaluation.

  // reuse of futures: have to reset the state after value was received, should be atomic and serve as abritation? possible race: receiver did not have time to read value before the future is reused by another control flow. solution would be to bind the future to a user's identifier (acquire/release) without implicit release after first read. use RAII for this.

  // based on scala: future is read-only container for async result. promise is write-once channel to put value into a future. multiple promises may be associated with the same future. other systems: one-to-one association between future and promise, that is each future contains one promise and it is handed over to the process that computes the value.

  /// @todo not safe against concurrent access
  class FutureBase
  {
  public:
    FutureBase() {}
    FutureBase(FutureBase const&) = delete; // don't copy
    FutureBase(FutureBase&&) = delete; // don't move

    Error error() const { return _error; }
    bool operator! () const { return _error == Error::UNSET; }
    explicit operator bool() const { return _error != Error::UNSET; }

    bool poll();
    bool wait();

    void then(runtime::Tasklet& t) {
      ASSERT(!waiting);
      waiting = &t ; /// @todo support list of multiple waiters?
      // check after appending in order to not miss concurrent assign
      if (_error != Error::UNSET) signal();
    }

  protected:
    void reset() { _error = Error::UNSET; waiting = nullptr; }
    void assign(Error e) { _error = e; signal(); }
    void signal();

  protected:
    Error _error = Error::UNSET;
    runtime::Tasklet* waiting = nullptr;
  };

} // namespace mythos
