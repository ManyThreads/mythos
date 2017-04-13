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
 * Copyright 2016 Randolf Rotta, Robert Kuban, and contributors, BTU Cottbus-Senftenberg
 */
#pragma once

#include "util/assert.hh"
#include "async/TaskletQueue.hh"
#include "async/DeletionMonitor.hh"
#include "async/Place.hh"

namespace mythos {
namespace async {

class NestedMonitorDelegating
  : public DeletionMonitor
{
public:
  NestedMonitorDelegating() : home(nullptr) {}

  /** for compatibility with @c NestedMonitorHome */
  NestedMonitorDelegating(Place*) : home(nullptr) {}

  /** for compatibility with @c NestedMonitorHome */
  void setHome(Place*) {}

  template<class FUNCTOR>
  void request(Tasklet* msg, FUNCTOR fun) { request(msg->set(fun)); }
  void request(Tasklet* msg);
  void requestDone();

  template<class FUNCTOR>
  void response(Tasklet* msg, FUNCTOR fun) {
    Place* myPlace = home.load(std::memory_order_relaxed);
    ASSERT(myPlace != nullptr);
    myPlace->run(msg->set(fun), Place::MAYINLINE);
  }

  void responseDone() {}
  void responseAndRequestDone() { requestDone(); }

protected:
  std::atomic<Place*> home; // used to forward responses to the right place
  TaskletQueue waitq;
};

} // namespace async
} // namespace mythos
