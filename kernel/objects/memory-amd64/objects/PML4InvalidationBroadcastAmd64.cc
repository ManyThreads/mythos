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

#include "objects/PML4InvalidationBroadcastAmd64.hh"

#include "cpu/hwthreadid.hh"
#include "boot/pagetables.hh"

namespace mythos {

  PML4InvalidationBroadcast PML4InvalidationBroadcast::nodes[MYTHOS_MAX_THREADS];

  void PML4InvalidationBroadcast::init()
  {
    for (cpu::ThreadID id = 0; id < cpu::getNumThreads(); ++id) {
      /// @todo not needed with linear thread id!
      nodes[id].next = &nodes[(id+1)%cpu::getNumThreads()];
      nodes[id].home = async::getPlace(id);
    }
  }

  void PML4InvalidationBroadcast::run(Tasklet* t, IResult<void>* res, PhysPtr<void> pml4)
  {
    MLOG_DETAIL(mlog::cap, "start pml4 invalidation broadcast");
    PML4InvalidationBroadcast* start = &nodes[cpu::getThreadID()];
    start->broadcast(t, res, pml4, start);
  }

  void PML4InvalidationBroadcast::broadcast(Tasklet* t, IResult<void>* res, PhysPtr<void> pml4, PML4InvalidationBroadcast* start)
  {
    // invalidate if neccessary
    PhysPtr<void> kernel_pml4(boot::table_to_phys_addr(boot::pml4_table, 1));
    if (getLocalPlace().getCR3() == pml4) {
      getLocalPlace().setCR3(kernel_pml4);
    }
    // propagate
    PML4InvalidationBroadcast* pnext = this->next;
    while (pnext != start && pnext->home->getCR3() != pml4) pnext = pnext->next;
    if (pnext != start) {
      MLOG_DETAIL(mlog::cap, "relay pml4 invalidation");
      pnext->home->run(t->set( [=](Tasklet* t){ pnext->broadcast(t, res, pml4,  start); } ));
    } else {
      MLOG_DETAIL(mlog::cap, "end pml4 invalidation");
      res->response(t);
    }
  }

} // namespace mythos

