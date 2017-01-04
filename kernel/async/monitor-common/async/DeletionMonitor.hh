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

#include <atomic>
#include "async/Place.hh"
#include "async/Tasklet.hh"
#include "async/mlog.hh"

namespace mythos {
namespace async {

class DeletionMonitor
{
public:
  DeletionMonitor() : refcount(0), deleteTask(nullptr) {}

  void acquireRef() { refcount++; }

  void releaseRef() {
    auto result = refcount.fetch_sub(1);
    if (result == 0 && deleteTask != nullptr) {
      MLOG_DETAIL(mlog::async, this, "schedule delete task");
      getLocalPlace().pushShared(deleteTask);
    }
  }

  template<class FUNCTOR>
  void doDelete(Tasklet* msg, FUNCTOR fun) {
    refcount++;
    deleteTask = msg->set(fun);
    if (--refcount == 0) {
      MLOG_DETAIL(mlog::async, this, "schedule delete task immediately");
      getLocalPlace().pushShared(deleteTask);
    }
  }

protected:
  std::atomic<size_t> refcount;
  std::atomic<Tasklet*> deleteTask;
};

} // async
} //mythos

