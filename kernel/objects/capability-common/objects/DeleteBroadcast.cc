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

#include "objects/DeleteBroadcast.hh"
#include "cpu/hwthreadid.hh"
#include "objects/mlog.hh"

namespace mythos {

  DeleteBroadcast DeleteBroadcast::nodes[BOOT_MAX_THREADS];

  void DeleteBroadcast::init()
  {
    ASSERT(cpu::hwThreadCount() > 0);
    for (size_t i = 0; i < cpu::hwThreadCount(); ++i) {
      auto id = cpu::enumerateHwThreadID(i);
      auto nextId = cpu::enumerateHwThreadID((i+1)%cpu::hwThreadCount());
      nodes[id].next = &nodes[nextId];
      nodes[id].home = async::getPlace(id);
    }
  }

  void DeleteBroadcast::run(Tasklet* t, IResult<void>* res)
  {
    mlog::cap.detail("start delete broadcast");
    DeleteBroadcast* start = &nodes[cpu::hwThreadID()];
    start->broadcast(t, res, start);
  }

  void DeleteBroadcast::broadcast(Tasklet* t, IResult<void>* res, DeleteBroadcast* start)
  {
    DeleteBroadcast* pnext = this->next;
    while (pnext != start && !pnext->home->isActive()) pnext = pnext->next;
    if (pnext != start) {
      mlog::cap.detail("relay delete broadcast");
      pnext->home->run(t->set( [=](Tasklet* t){ broadcast(t, res, start); } ));
    } else {
      mlog::cap.detail("end delete broadcast");
      res->response(t);
    }
  }

} // namespace mythos
