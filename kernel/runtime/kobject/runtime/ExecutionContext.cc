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

#include "runtime/ExecutionContext.hh"

namespace mythos {

  PortalFutureRef<void>
  ExecutionContext::create(PortalRef pr, UntypedMemory kmem, CapPtr factory,
                          PageMap as, CapMap cs, CapPtr sched,
                          void* stack, StartFun start, void* userctx)
  {
    if (!pr.isOpen()) return pr;
    // prepare stack
    uintptr_t* tos = reinterpret_cast<uintptr_t*>(uintptr_t(stack) & ~0xF); // align
    *--tos = 0; // dummy return address
    // prepare message and registers
    auto msg = pr.write<protocol::ExecutionContext::Create>(_cap, factory, as.cap(), cs.cap(), sched, true);
    msg->regs.rdi = uintptr_t(start); // arg1
    msg->regs.rsi = uintptr_t(userctx); // arg2
    msg->regs.rip = uintptr_t(&ExecutionContext::start);
    msg->regs.rsp = uintptr_t(tos);
    msg->regs.fs_base = 0; /// @todo set fs, gs according to environment
    msg->regs.gs_base = 0;
    pr.invoke(kmem.cap());
    return pr;
  }

  void ExecutionContext::start(StartFun main, void* userctx)
  {
    syscall_exit(uintptr_t(main(userctx)));
  }

  PortalFutureRef<void>
  ExecutionContext::configure(PortalRef pr, PageMap as, CapMap cs, CapPtr sched)
  {
    return pr.tryInvoke<protocol::ExecutionContext::Configure>(_cap, as.cap(), cs.cap(), sched);
  }

  ExecutionContext::RegsRef ExecutionContext::readRegisters(PortalRef pr, bool suspend)
  {
    return pr.tryInvoke<protocol::ExecutionContext::ReadRegisters>(_cap, suspend);
  }

  PortalFutureRef<void> ExecutionContext::writeRegisters(PortalRef pr, register_t regs, bool resume)
  {
    return pr.tryInvoke<protocol::ExecutionContext::WriteRegisters>(_cap, resume, regs);
  }

  PortalFutureRef<void> ExecutionContext::setFSGS(PortalRef pr, uintptr_t fs, uintptr_t gs)
  {
    return pr.tryInvoke<protocol::ExecutionContext::SetFSGS>(_cap, fs,gs);
  }

  PortalFutureRef<void> ExecutionContext::resume(PortalRef pr)
  {
    return pr.tryInvoke<protocol::ExecutionContext::Resume>(_cap);
  }

  PortalFutureRef<void> ExecutionContext::suspend(PortalRef pr)
  {
    return pr.tryInvoke<protocol::ExecutionContext::Suspend>(_cap);
  }

} // namespace mythos
