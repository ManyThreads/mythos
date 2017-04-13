/* -*- mode:C++; indent-tabs-mode:nil; -*- */
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
 * Copyright 2016 Randolf Rotta, Robert Kuban, and contributors, BTU Cottbus-Senftenberg
 */
#pragma once

#include "util/assert.hh"
#include <atomic>
#include <new>
#include <cstddef>
#include <utility>

namespace mythos {

  /** Tasklet implementation. All users of Tasklets expect at least the size of one cacheline. */
  class alignas(64) Tasklet
  {
  public:
    typedef void (*FunPtr)(Tasklet&);

    constexpr static uintptr_t FREE = 0; // used by TaskletQueue
    constexpr static uintptr_t INCOMPLETE = 1; // used by TaskletQueue: insertion is still in progress
    constexpr static uintptr_t LOCKED = 2; // used by TaskletQueue
    constexpr static uintptr_t UNUSED = 3; // the Tasklet is neither initialised nor in a queue
    constexpr static uintptr_t INIT = 4; // the Tasklet is initialised
    constexpr static size_t CLSIZE = 64;
    constexpr static size_t PAYLOAD_SIZE = CLSIZE - sizeof(std::atomic<uintptr_t>) - sizeof(FunPtr);

    Tasklet() : nextTasklet(UNUSED), handler(nullptr) {}
    Tasklet(const Tasklet&) = delete;
    ~Tasklet() { ASSERT(isUnused()); }

#ifdef NDEBUG
    bool isInit() { return true; }
    void setInit() {}
    bool isUnused() { return true; }
    void setUnused() {}
#else
    bool isInit() { return nextTasklet == INIT; }
    void setInit() { nextTasklet = INIT; }
    bool isUnused() { return nextTasklet == UNUSED; }
    void setUnused() { nextTasklet = UNUSED; }
#endif

    template<class FUNCTOR>
    Tasklet& operator<< (FUNCTOR const&& fun) { set(std::move(fun)); return *this; }

    void operator() () {
      ASSERT(isInit());
      setUnused();
      handler(*this);
    }

  protected:
    template<class FUNCTOR>
    void set(FUNCTOR const&& fun) {
      static_assert(sizeof(FUNCTOR) <= sizeof(payload), "tasklet payload is too big");
      ASSERT(isUnused());
      setInit();
      new(payload) FUNCTOR(std::move(fun)); // copy-construct
      handler = &wrapper<FUNCTOR>;
    }

    template<class MSG>
    MSG get() const {
      static_assert(sizeof(MSG) <= sizeof(payload), "tasklet payload is too big");
      union cast_t {
	char payload[PAYLOAD_SIZE];
	MSG msg;
      };
      cast_t const* caster = reinterpret_cast<cast_t const*>(payload);
      return MSG(std::move(caster->msg));
    }

    template<class FUNCTOR>
    static void wrapper(Tasklet& msg) { msg.get<FUNCTOR>()(msg); }

  public:
    std::atomic<uintptr_t> nextTasklet;
  private:
    FunPtr handler;
    char payload[PAYLOAD_SIZE];
  };

} // namespace mythos
