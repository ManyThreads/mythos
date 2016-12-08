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
#include "async/DelegationQueue.hh"
#include "async/Place.hh"
#include "async/DeletionMonitor.hh"

namespace mythos {
namespace async {

class MonitorDelegating
  : public DeletionMonitor
{
public:
  MonitorDelegating() : inResponse(false) {}

  template<class FUNCTOR>
  void request(Tasklet* msg, FUNCTOR fun)
  {
    msg->set(fun);
    if (requestQueue.push(msg)) { // first
      msg = requestQueue.pop();
      enqueue(msg);
    }
    acquireRef();
  }

  void continueRequest()
  {
    release();
  }

  void requestDone()
  {
    if (!requestQueue.tryRelease()) {
      auto msg = requestQueue.pop();
      mlog::async.info(this, "after pop");
      enqueue(msg);
    }
    // if in response, the response is responsible
    // for releasing the monitor
    if (!inResponse) {
      mlog::async.info(this, "release in request");
      release();
    } else {
      mlog::async.info(this, "release in response");
    }
    releaseRef();
  }

  template<class FUNCTOR>
  void response(Tasklet* msg, FUNCTOR fun)
  {
    acquireRef();
    msg->set(fun);
    ASSERT(!inResponse);
    inResponse = true;
    enqueue(msg);
  }

  void responseDone()
  {
    release();
    ASSERT(inResponse);
    inResponse = false;
    releaseRef();
  }
  
protected:
  void enqueue(Tasklet* msg)
  {
    if (queue.push(msg)) { // first
      mlog::async.info(this, "acquired monitor");
      msg = queue.pop();
      getLocalPlace().runLocal(msg);
    } else {
        mlog::async.info(this, "delegated monitor");
    }
  }

  void release() {
    if (!queue.tryRelease()) {
      mlog::async.info(this, "reschedule monitor");
      auto msg = queue.pop();
      getLocalPlace().runLocal(msg);
    } else {
      mlog::async.info(this, "released monitor");
    }
  }

  // XXX fails if multiple sub-requests return their responses concurrently, but will not harm...
  // should be protected by the monitor -> only altered when response is actually executed
  bool inResponse;

  DelegationQueueLIFO requestQueue; //< all unprocessed requests
  DelegationQueueLIFO queue; //< single request and all unprocessed responses
};

} // async
} //mythos
