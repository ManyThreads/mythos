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
 * Copyright 2019 Randolf Rotta, Robert Kuban, and contributors, BTU Cottbus-Senftenberg
 */
#pragma once

#include "util/assert.hh"

namespace mythos {

enum class EventCtrl { OK, STOP, PREVENT_DEFAULT, STOP_AND_PREVENT };

template<typename... Args>
class Event;

/** single-linked intrusive list with priority-based insertion. */
template<typename... Args>
class EventHook
{
public:
  EventHook() {}
  virtual ~EventHook() {}
  virtual int priority() const { return 0; }
  virtual EventCtrl before(Args...) { return EventCtrl::OK; }
  virtual EventCtrl after(Args...) { return EventCtrl::OK; }
private:
  friend class Event<Args...>;
  EventHook<Args...>* next = {nullptr};
};

/** Typed registry for hooks into a certain event.
 * Manages a single-linked list of hooks ordered by their priority.
 * Inspired by dokuwiki's event hooks.
 *
 * @todo Should be protected against concurrency? actually a reader-writer lock,
 * but only if we allow dynamic insert/remove after startup.
 */
template<typename... Args>
class Event
{
public:
  typedef EventHook<Args...> hook_t;

  Event() {}

  /** insert after this object but before the next object with
   * lower priority. */
  void add(hook_t* ev);

  template<class FUN>
  Event& trigger(FUN const& fun, Args... args) {
    trigger(hooks, true, fun, args...);
    return *this;
  }

  Event& trigger(Args... args) { return trigger([](Args...){}, args...); }

protected:
  template<class FUN>
  bool trigger(hook_t* h, bool doDefault, FUN const& fun, Args... args);

private:
  hook_t* hooks = {nullptr};
};

template<typename... Args>
void Event<Args...>::add(hook_t* ev)
{
  ASSERT(ev && !ev->next);
  auto cur = &hooks;
  while (*cur && (*cur)->priority() > ev->priority()) cur = &(*cur)->next;
  ev->next = *cur;
  *cur = ev;
}

template<typename... Args>
template<class FUN>
bool Event<Args...>::trigger(hook_t* h, bool doDefault, FUN const& fun, Args... args)
{
    if (!h) {
        if (doDefault) fun(args...);
        return true;
    }
    bool doAfter;
    switch (h->before(args...)) {
    case EventCtrl::STOP: doAfter = trigger(nullptr, doDefault, fun, args...); break;
    case EventCtrl::PREVENT_DEFAULT: doAfter = trigger(h->next, false, fun, args...); break;
    case EventCtrl::STOP_AND_PREVENT: doAfter = trigger(nullptr, false, fun, args...); break;
    default: doAfter = trigger(h->next, doDefault, fun, args...);
    }
    if (!doAfter) return false;
    switch (h->after(args...)) {
    case EventCtrl::STOP: return false;
    case EventCtrl::STOP_AND_PREVENT: return false;
    default: return true;
    }
}

} // namespace mythos
