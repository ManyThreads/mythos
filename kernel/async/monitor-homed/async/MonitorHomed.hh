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

#include "util/assert.hh"
#include "async/Queues.hh"
#include "async/Place.hh"

namespace mythos {
namespace async {

class MonitorHomed
{
public:

  // MonitorHomed()
  //   : home(nullptr), pendingRequests(0), refcount(0), deleteTask(nullptr)
  // {}

  MonitorHomed(Place* home)
    : home(home), pendingRequests(0), refcount(0), deleteTask(nullptr)
  {}

  void setHome(Place* p)
  {
    ASSERT(!home);
    ASSERT(p);
    home = p;
  }

  template<class FUNCTOR>
  void request(Tasklet* msg, FUNCTOR fun) {
    ASSERT(home);
    size_t oldReq = pendingRequests.fetch_add(1);
    if (oldReq == 0) {
      // this is the only thread that was first, now pendingRequests > 0
      // forward the first request to the home thread
      home->pushShared(msg->set(fun));
    } else {
      // other requests were pending, thus store own request in monitor's queue.
      sharedQueue.push(msg->set(fun));
    }
  }

  void requestDone() {
    // This is executed on the home thread.
    // If a pending request is enqueued, we forward the oldest to home's private queue.
    // Another thread could enqueue a request concurrently.
    size_t oldReq = pendingRequests.fetch_add(-1);
    if (oldReq > 1) {
      // we are responsible to schedule the next tasklet
      // will have to wait if it is not yet in the shared queue
      while (true) {
        Tasklet* msg = privateQueue.pop();
        if (msg != nullptr) {
          home->runLocal(msg);
          return;
        } else {
          sharedQueue.retrieve(privateQueue);
          hwthread_pause();
        }
      }
    } else {
      // this was the last pending request, check for deletion
      if (refcount == 0 && deleteTask != nullptr) home->pushShared(deleteTask);
    }
  }

  template<class FUNCTOR>
  void response(Tasklet* msg, FUNCTOR fun) {
    ASSERT(home);
    acquireRef();
    home->run(msg->set(fun));
  }

  void responseDone() { releaseRef(); }

  void acquireRef() { refcount++; }
  void releaseRef() {
    refcount--;
    if (refcount == 0 && pendingRequests==0 && deleteTask != nullptr)
      home->pushShared(deleteTask);
  }

  template<class FUNCTOR>
  void doDelete(Tasklet* msg, FUNCTOR fun) {
    ASSERT(home);
    deleteTask = msg->set(fun);
    if (refcount == 0 && pendingRequests==0) home->pushShared(deleteTask);
  }


protected:
  Place* home;

  std::atomic<size_t> pendingRequests;
  PrivateTaskletQueue privateQueue; //< accessed only by the owner thread
  SharedTaskletQueue sharedQueue; //< for incoming tasks from other threads

  std::atomic<size_t> refcount;
  std::atomic<Tasklet*> deleteTask;
};

} // namespace async
} // namespace mythos
