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
#pragma once

#include "async/NestedMonitorDelegating.hh"
#include "objects/IFactory.hh"
#include "objects/IKernelObject.hh"
#include "cpu/hwthreadid.hh"
#include "mythos/protocol/ProcessorAllocator.hh"
#include "boot/mlog.hh"
#include "objects/RevokeOperation.hh"
#include "async/IResult.hh"

namespace mythos {

class ProcessorAllocator
  : public IKernelObject
{
  public:
    ProcessorAllocator();

  /* IKernelObject */
    optional<void> deleteCap(CapEntry&, Cap self, IDeleter& del) override;
    void deleteObject(Tasklet* t, IResult<void>* r) override;
    optional<void const*> vcast(TypeId id) const override {
      if (id == typeId<ProcessorAllocator>()) return this;
      THROW(Error::TYPE_MISMATCH);
    }

    void init();

    //todo: synchronize
    optional<cpu::ThreadID> alloc(); 
    void free(cpu::ThreadID id);

    CapEntry* getSC(cpu::ThreadID id){
      ASSERT(id < cpu::getNumThreads());
      return &sc[id];
    }

  protected:
    async::NestedMonitorDelegating monitor;
    CapEntry *sc;
    CapEntry mySC[MYTHOS_MAX_THREADS];
    unsigned nFree;
    cpu::ThreadID freeList[MYTHOS_MAX_THREADS];
};
} // namespace mythos
