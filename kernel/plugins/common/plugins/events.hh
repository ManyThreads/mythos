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
 * Copyright 2017 Randolf Rotta, Robert Kuban, and contributors, BTU Cottbus-Senftenberg
 */
#pragma once

#include "util/assert.hh"

namespace mythos {

  template<class Event>
  class HookRegistry;

  enum class EventCtrl {
    OK, STOP, PREVENT_DEFAULT, STOP_AND_PREVENT
  };

  template<class Event>
  class EventHook
  {
  public:
    EventHook() : next(nullptr) {}
    virtual ~EventHook() {}
    //virtual int priority() const { return 0; }

    virtual EventCtrl before(Event&) { return EventCtrl::OK; }
    virtual EventCtrl after(Event&) { return EventCtrl::OK; }

  protected:
    friend class HookRegistry<Event>;
    EventHook* next;
  };

  /** inspired by dokuwiki's event hooks */
  template<class Event>
  class HookRegistry
  {
  public:
    HookRegistry() : first(nullptr) {}

    void register_hook(EventHook<Event>* hook);

    /** process 'before' handlers, return false if default action should be prevented */
    bool trigger_before(Event& event);

    /** process 'after' handlers */
    void trigger_after(Event& event);

  protected:
    EventHook<Event>* first;
  };

  template<class Event>
  inline void HookRegistry<Event>::register_hook(EventHook<Event>* hook)
  {
    ASSERT(hook);
    hook->next = first;
    first = hook;
  }

  template<class Event>
  bool HookRegistry<Event>::trigger_before(Event& event)
  {
    bool ok = true;
    for (EventHook<Event>* p = first; p; p = p->next) {
      switch (p->before(event)) {
      case EventCtrl::STOP: return ok;
      case EventCtrl::PREVENT_DEFAULT: ok = false; break;
      case EventCtrl::STOP_AND_PREVENT: return false;
      default:;
      }
    }
    return ok;
  }

  template<class Event>
  void HookRegistry<Event>::trigger_after(Event& event) {
    for (EventHook<Event>* p = first; p; p = p->next) {
      switch (p->after(event)) {
      case EventCtrl::STOP:
      case EventCtrl::STOP_AND_PREVENT: return;
      default:;
      }
    }
  }

} // namespace mythos
