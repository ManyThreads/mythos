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
 * Copyright 2016 Randolf Rotta, Robert Kuban, and contributors, BTU Cottbus-Senftenberg
 */
#pragma once

#include <atomic>
#include <new>
#include "async/mlog.hh"
#include "util/hash.hh"

namespace mythos {

namespace async {

class Chainable
{
public:
  constexpr static uintptr_t FREE = 0; // used by TaskletQueue
  constexpr static uintptr_t INCOMPLETE = 1; // used by TaskletQueue: insertion is still in progress
  constexpr static uintptr_t LOCKED = 2; // used by TaskletQueue
  constexpr static uintptr_t UNUSED = 3; // is neither initialised nor in a queue
  constexpr static uintptr_t INIT = 4; // is initialised
  constexpr static uintptr_t INIT_HANDOVER = 5; // other thread shall take over

  Chainable() {}
  Chainable(Chainable&& o) : next(o.next.load(std::memory_order_relaxed)) {
    //o.setUnused();
  }

#ifdef NDEBUG
  bool isInit() const { return true; }
  void setInit() {}
  bool isHandover() const { return next == INIT_HANDOVER; }
  void setHandover() { next = INIT_HANDOVER; }
  bool isUnused() const { return true; }
  void setUnused() {}
#else
  bool isInit() const { uintptr_t n = next; return n == INIT || n == INIT_HANDOVER; }
  void setInit() { next = INIT; }
  bool isHandover() const { return next == INIT_HANDOVER; }
  void setHandover() { ASSERT(next == INIT); next = INIT_HANDOVER; }
  bool isUnused() const { return next == UNUSED; }
  void setUnused() { next = UNUSED; }
#endif

public:
  std::atomic<uintptr_t> next = {UNUSED};
};


/** Base class for dummy Tasklet objects, which are sometimes needed for queue management. */
class TaskletBase
  : public Chainable
{
public:
  typedef void(*FunPtr)(TaskletBase*);

  TaskletBase() {}
  TaskletBase(FunPtr handler) : handler(handler) { setInit(); }
  TaskletBase(TaskletBase&& o) : Chainable(std::move(o)), handler(o.handler) {}
  ~TaskletBase() { ASSERT(isUnused()); }

  void run() {
    MLOG_DETAIL(mlog::async, "run tasklet", this);
    ASSERT(handler > FunPtr(VIRT_ADDR));
    ASSERT(isInit());
    setUnused();
    handler(this);
  }

protected:
  FunPtr handler = nullptr;
};


/** Actual Tasklet implementation. All users of Tasklets expect at least the size of one cacheline. */
class alignas(64) Tasklet
  : public TaskletBase
{
public:
  static constexpr size_t CLSIZE = 64;
  static constexpr size_t PAYLOAD_SIZE = CLSIZE - sizeof(TaskletBase);

  Tasklet() {}
  Tasklet(const Tasklet&) = delete;

  template<class FUNCTOR>
  Tasklet* operator<< (FUNCTOR fun) { return set(fun); }

  template<class FUNCTOR>
  Tasklet* set(FUNCTOR fun) {
    static_assert(sizeof(FUNCTOR) <= sizeof(payload), "tasklet payload is too big");
    ASSERT(isUnused());
    setInit();
    new(payload) FUNCTOR(std::move(fun));
    this->handler = &wrapper<FUNCTOR>;
    return this;
  }

protected:
  // return the function object by copy!
  template<class MSG>
  MSG get() {
    static_assert(sizeof(MSG) <= sizeof(payload), "tasklet payload is too big");
    union cast_t {
      char payload[PAYLOAD_SIZE];
      MSG msg;
    };
    cast_t* caster = reinterpret_cast<cast_t*>(payload);
    return MSG(std::move(caster->msg));
  }

public:
  void print()
  {
    MLOG_INFO(mlog::async, "tasklet", this, "handler", handler, "hash", hash32(payload, 48));
    for (auto row = 0; row < 6; ++row) {
      MLOG_DETAIL(mlog::async, DMDUMP(&payload[row*8], 8));
    }
  }

protected:
  template<class FUNCTOR>
  static void wrapper(TaskletBase* msg)
  {
    auto t = static_cast<Tasklet*>(msg);
    t->get<FUNCTOR>()(t);
  }

private:
  char payload[PAYLOAD_SIZE];
};


template<class FUNCTOR>
class alignas(64) TaskletFunctor
  : public TaskletBase
{
public:
  static constexpr size_t CLSIZE = 64;

  TaskletFunctor(FUNCTOR fun) : TaskletBase(&wrapper), fun(fun) {
    static_assert(sizeof(TaskletFunctor)%CLSIZE == 0, "padding is wrong");
  }
  TaskletFunctor(const TaskletFunctor&) = delete;
  TaskletFunctor(TaskletFunctor&& o) : TaskletBase(std::move(o)), fun(std::move(o.fun)) {}

protected:
  static void wrapper(TaskletBase* msg) { static_cast<TaskletFunctor*>(msg)->fun(msg); }

protected:
  FUNCTOR fun;
  char padding[CLSIZE - (sizeof(FUNCTOR)+sizeof(TaskletBase))%CLSIZE];
};

template<class FUNCTOR>
TaskletFunctor<FUNCTOR> makeTasklet(FUNCTOR fun) { return TaskletFunctor<FUNCTOR>(fun); }

} // namespace async

using async::Tasklet;

} // namespace mythos
