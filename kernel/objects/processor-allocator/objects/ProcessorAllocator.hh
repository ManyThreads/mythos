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
 * Copyright 2020 Philipp Gypser and contributors, BTU Cottbus-Senftenberg
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
  , public IResult<void>
{
  public:
    ProcessorAllocator();

  /* IKernelObject */
    optional<void> deleteCap(CapEntry&, Cap self, IDeleter& del) override;
    void deleteObject(Tasklet* t, IResult<void>* r) override;
    void invoke(Tasklet* t, Cap self, IInvocation* msg) override;

  /* IResult<void> */
    void response(Tasklet* /*t*/, optional<void> res) override;

    void init();
    Error invokeAlloc(Tasklet*, Cap, IInvocation* msg);
    Error invokeFree(Tasklet* t, Cap, IInvocation* msg);
    void freeSC(Tasklet* t, cpu::ThreadID id);

  protected:
    friend class PluginProcessorAllocator;
    virtual optional<cpu::ThreadID> alloc() = 0;
    virtual void free(cpu::ThreadID id) = 0;
    virtual unsigned numFree() = 0;

  private:
    async::NestedMonitorDelegating monitor;
    RevokeOperation revokeOp = {monitor};
    cpu::ThreadID toBeFreed = 0;
    CapEntry *sc;
    CapEntry mySC[MYTHOS_MAX_THREADS];
};

class LiFoProcessorAllocator : public ProcessorAllocator
{
  public:
    LiFoProcessorAllocator();

    unsigned numFree() override { return nFree; }
    optional<cpu::ThreadID> alloc() override; 
    void free(cpu::ThreadID id) override;

  private:
    unsigned nFree;
    cpu::ThreadID freeList[MYTHOS_MAX_THREADS];
};

} // namespace mythos
