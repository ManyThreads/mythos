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
 * Copyright 2020 Philipp Gypser, BTU Cottbus-Senftenberg
 */


#include "objects/ProcessorAllocator.hh"
#include "objects/mlog.hh"

namespace mythos {

/* IKernelObject */ 
  optional<void> ProcessorAllocator::deleteCap(CapEntry&, Cap, IDeleter&)
  {
    MLOG_DETAIL(mlog::pm, __func__);
    RETURN(Error::SUCCESS);
  }

  void ProcessorAllocator::deleteObject(Tasklet*, IResult<void>*)
  {
    MLOG_DETAIL(mlog::pm, __func__);
  }

/* ProcessorAllocator */
  ProcessorAllocator::ProcessorAllocator()
      : sc(image2kernel(&mySC[0]))
      , nTeams(0)
    {}

  void ProcessorAllocator::init(){
    MLOG_DETAIL(mlog::pm, "PM::init");
    for (cpu::ThreadID id = 0; id < cpu::getNumThreads(); ++id) {
      sc[id].initRoot(Cap(image2kernel(&boot::getScheduler(id))));
      free(id);
    }
  }

  optional<cpu::ThreadID> ProcessorAllocator::alloc(){
    MLOG_DETAIL(mlog::pm, __func__);
    optional<cpu::ThreadID> ret;
    if(nFree > 0){
      nFree--;
      ret = freeList[nFree];
    }
    return ret;
  }

  void ProcessorAllocator::free(cpu::ThreadID id) {
      MLOG_INFO(mlog::pm, __func__, DVAR(id));
      freeList[nFree] = id;
      nFree++;
  }
} // namespace mythos
