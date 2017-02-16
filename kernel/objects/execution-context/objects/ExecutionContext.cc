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

#include "objects/ExecutionContext.hh"

#include "cpu/kernel_entry.hh"
#include "cpu/ctrlregs.hh"
#include "objects/mlog.hh"
#include "objects/ops.hh"
#include "objects/DebugMessage.hh"
#include "objects/IPageMap.hh"
#include "util/error-trace.hh"

namespace mythos {

  ExecutionContext::ExecutionContext(IAsyncFree* memory)
    :  flags(0), memory(memory)
  {
    setFlag(IS_EXITED | NO_AS | NO_SCHED);
    memset(&threadState, 0, sizeof(threadState));
  }

  void ExecutionContext::setFlagSuspend(uint8_t f)
  {
    auto prev = setFlag(f);
    if (!isBlocked(prev) && !isReady()) {
      auto sched = _sched.get();
      if (sched) sched->preempt(&ec_handle);
    }
  }

  void ExecutionContext::clearFlagResume(uint8_t f)
  {
    auto prev = clearFlag(f);
    MLOG_INFO(mlog::ec, "cleared flag", DVAR(this), DVARhex(f), DVARhex(prev), DVARhex(flags.load()), isReady());
    if (isBlocked(prev) && isReady()) {
      auto sched = _sched.get();
      MLOG_INFO(mlog::ec, "trying to wake up the scheduler");
      if (sched) sched->ready(&ec_handle);
    }
  }

  optional<void> ExecutionContext::setAddressSpace(optional<CapEntry*> pme)
  {
    MLOG_INFO(mlog::ec, "setAddressSpace", DVAR(this), DVAR(pme));
    TypedCap<IPageMap> obj(pme);
    if (!obj) RETHROW(obj);
    if (!obj->getPageMapInfo(obj.cap()).isRootMap()) THROW(Error::INVALID_CAPABILITY);
    RETURN(_as.set(this, *pme, obj.cap()));
  }

  void ExecutionContext::bind(optional<IPageMap*>)
  {
    MLOG_INFO(mlog::ec, "setAddressSpace bind", DVAR(this));
    clearFlagResume(NO_AS);
  }

  void ExecutionContext::unbind(optional<IPageMap*>)
  {
    MLOG_INFO(mlog::ec, "setAddressSpace unbind", DVAR(this));
    setFlagSuspend(NO_AS);
  }

  optional<void> ExecutionContext::setSchedulingContext(optional<CapEntry*> sce)
  {
    MLOG_INFO(mlog::ec, "setScheduler", DVAR(this), DVAR(sce));
    TypedCap<IScheduler> obj(sce);
    if (!obj) RETHROW(obj);
    RETURN(_sched.set(this, *sce, obj.cap()));
  }

  void ExecutionContext::bind(optional<IScheduler*>)
  {
    MLOG_INFO(mlog::ec, "setScheduler bind", DVAR(this));
    clearFlagResume(NO_SCHED);
  }

  void ExecutionContext::unbind(optional<IScheduler*> sched)
  {
    MLOG_INFO(mlog::ec, "setScheduler unbind", DVAR(this));
    ASSERT(sched);
    setFlag(NO_SCHED);
    sched->unbind(&ec_handle);
  }

  optional<void> ExecutionContext::setCapSpace(optional<CapEntry*> cse)
  {
    MLOG_INFO(mlog::ec, "setCapSpace", DVAR(this), DVAR(cse));
    TypedCap<ICapMap> obj(cse);
    if (!obj) RETHROW(obj);
    RETURN(_cs.set(this, *cse, obj.cap()));

  }

  void ExecutionContext::setEntryPoint(uintptr_t rip)
  {
    MLOG_INFO(mlog::ec, "EC setEntryPoint", DVARhex(rip));
    threadState.rip = rip;
    clearFlagResume(IS_EXITED);
  }

  optional<CapEntryRef> ExecutionContext::lookupRef(CapPtr ptr, CapPtrDepth ptrDepth, bool writable)
  {
    TypedCap<ICapMap> cs(_cs.cap());
    if (!cs) RETHROW(cs);
    RETURN(cs.lookup(ptr, ptrDepth, writable));
  }

  Error ExecutionContext::invokeConfigure(Tasklet*, Cap, IInvocation* msg)
  {
    auto data = msg->getMessage()->read<protocol::ExecutionContext::Configure>();

    if (data.as() == delete_cap) { unsetAddressSpace(); }
    else if (data.as() != null_cap) {
      auto err = setAddressSpace(msg->lookupEntry(data.as()));
      if (!err) return err.state();
    }

    if (data.cs() == delete_cap) { unsetCapSpace(); }
    else if (data.cs() != null_cap) {
      auto err = setCapSpace(msg->lookupEntry(data.cs()));
      if (!err) return err.state();
    }

    if (data.sched() == delete_cap) { unsetSchedulingContext(); }
    if (data.sched() != null_cap) {
      auto err = setSchedulingContext(msg->lookupEntry(data.sched()));
      if (!err) return err.state();
    }

    return Error::SUCCESS;
  }

  Error ExecutionContext::invokeReadRegisters(Tasklet* t, Cap, IInvocation* msg)
  {
    this->msg = msg;
    auto const& data = *msg->getMessage()->cast<protocol::ExecutionContext::ReadRegisters>();
    if (data.suspend) setFlag(IS_TRAPPED);
    setFlag(REGISTER_ACCESS);
    auto sched = _sched.get();
    if (sched) sched->preempt(t, &readSleepResponse, &ec_handle);
    else readThreadRegisters(t, optional<void>(Error::SUCCESS));
    return Error::INHIBIT;
  }

  void ExecutionContext::readThreadRegisters(Tasklet* t, optional<void>)
  {
    ASSERT(msg);
    monitor.response(t,[=](Tasklet*){
        auto data = msg->getMessage()->read<protocol::ExecutionContext::ReadRegisters>();
        auto respData = msg->getMessage()->write<protocol::ExecutionContext::WriteRegisters>(!data.suspend);
        respData->regs.rax = threadState.rax;
        respData->regs.rbx = threadState.rbx;
        respData->regs.rcx = threadState.rcx;
        respData->regs.rdx = threadState.rdx;
        respData->regs.rdi = threadState.rdi;
        respData->regs.rsi = threadState.rsi;
        respData->regs.r8  = threadState.r8;
        respData->regs.r9  = threadState.r9;
        respData->regs.r10 = threadState.r10;
        respData->regs.r13 = threadState.r13;
        respData->regs.r14 = threadState.r14;
        respData->regs.r15 = threadState.r15;
        respData->regs.rip = threadState.rip;
        respData->regs.rsp = threadState.rsp;
        respData->regs.rbp = threadState.rbp;
        respData->regs.fs_base = threadState.fs_base;
        respData->regs.gs_base = threadState.gs_base;
        clearFlagResume(REGISTER_ACCESS);
        msg->replyResponse(Error::SUCCESS);
        msg = nullptr;
        monitor.responseAndRequestDone();
      } );
  }

  Error ExecutionContext::invokeWriteRegisters(Tasklet* t, Cap, IInvocation* msg)
  {
    this->msg=msg;
    auto const& data = *msg->getMessage()->cast<protocol::ExecutionContext::WriteRegisters>();
    setFlag(REGISTER_ACCESS);
    if (data.resume) clearFlag(IS_TRAPPED);
    auto sched = _sched.get();
    if (sched) sched->preempt(t, &writeSleepResponse, &ec_handle);
    else writeThreadRegisters(t, optional<void>(Error::SUCCESS));
    return Error::INHIBIT;
  }

  void ExecutionContext::writeThreadRegisters(Tasklet* t, optional<void>)
  {
    ASSERT(msg);
    monitor.response(t,[=](Tasklet*){
        auto data = msg->getMessage()->read<protocol::ExecutionContext::WriteRegisters>();
        auto res = setRegisters(data.regs);
        if (!res) setFlag(IS_TRAPPED);
        clearFlagResume(REGISTER_ACCESS);
        msg->replyResponse(res);
        msg = nullptr;
        monitor.responseAndRequestDone();
      } );
  }

  optional<void> ExecutionContext::setRegisters(const mythos::protocol::ExecutionContext::Amd64Registers& regs)
  {
    threadState.rax = regs.rax;
    threadState.rbx = regs.rbx;
    threadState.rcx = regs.rcx;
    threadState.rdx = regs.rdx;
    threadState.rdi = regs.rdi;
    threadState.rsi = regs.rsi;
    threadState.r8  = regs.r8;
    threadState.r9  = regs.r9;
    threadState.r10 = regs.r10;
    threadState.r11 = regs.r11;
    threadState.r12 = regs.r12;
    threadState.r13 = regs.r13;
    threadState.r14 = regs.r14;
    threadState.r15 = regs.r15;
    threadState.rip = regs.rip;
    threadState.rsp = regs.rsp;
    threadState.rbp = regs.rbp;
    return setBaseRegisters(regs.fs_base, regs.gs_base);
  }

  optional<void> ExecutionContext::setBaseRegisters(uint64_t fs, uint64_t gs)
  {
    if (!PhysPtr<void>(fs).canonical() || !PhysPtr<void>(gs).canonical())
      THROW(Error::NON_CANONICAL_ADDRESS);
    threadState.fs_base = fs;
    threadState.gs_base = gs;
    RETURN(Error::SUCCESS);
  }

  Error ExecutionContext::invokeResume(Tasklet*, Cap, IInvocation*)
  {
    clearFlagResume(IS_TRAPPED);
    return Error::SUCCESS;
  }

  Error ExecutionContext::invokeSuspend(Tasklet* t, Cap, IInvocation* msg)
  {
    this->msg=msg;
    setFlag(IS_TRAPPED);
    auto sched = _sched.get();
    if (sched) sched->preempt(t, &sleepResponse, &ec_handle);
    else suspendThread(t, optional<void>(Error::SUCCESS));
    return Error::INHIBIT;
  }

  void ExecutionContext::suspendThread(Tasklet* t, optional<void>)
  {
    ASSERT(msg);
    monitor.response(t,[=](Tasklet*){
        msg->replyResponse(Error::SUCCESS);
        msg = nullptr;
        monitor.responseAndRequestDone();
      });
  }

  Error ExecutionContext::invokeSetFSGS(Tasklet*, Cap, IInvocation* msg)
  {
    auto const& data = *msg->getMessage()->cast<protocol::ExecutionContext::SetFSGS>();
    auto res = setBaseRegisters(data.fs, data.gs);
    if (res) { // reschedule the EC in order to load the new values
      setFlagSuspend(IS_TRAPPED);
      clearFlagResume(IS_TRAPPED);
    }
    return res.state();
  }

  void ExecutionContext::notify(INotifiable::handle_t* event)
  {
    MLOG_INFO(mlog::ec, "got notification", DVAR(this), DVAR(event));
    notificationQueue.push(event);
    clearFlagResume(IS_WAITING);
  }

  void ExecutionContext::denotify(INotifiable::handle_t* event)
  {
    notificationQueue.remove(event);
  }

  void ExecutionContext::handleTrap(cpu::ThreadState* ctx)
  {
    ASSERT(&threadState == ctx);
    MLOG_ERROR(mlog::ec, "user fault", DVAR(ctx->irq), DVAR(ctx->error),
         DVARhex(ctx->cr2));
    MLOG_ERROR(mlog::ec, "...", DVARhex(ctx->rip), DVARhex(ctx->rflags),
         DVARhex(ctx->rsp));
    MLOG_ERROR(mlog::ec, "...", DVARhex(ctx->rax), DVARhex(ctx->rbx), DVARhex(ctx->rcx),
         DVARhex(ctx->rdx), DVARhex(ctx->rbp), DVARhex(ctx->rdi),
         DVARhex(ctx->rsi));
    MLOG_ERROR(mlog::ec, "...", DVARhex(ctx->r8), DVARhex(ctx->r9), DVARhex(ctx->r10),
         DVARhex(ctx->r11), DVARhex(ctx->r12), DVARhex(ctx->r13),
         DVARhex(ctx->r14), DVARhex(ctx->r15));
    setFlag(IS_TRAPPED); // mark as not executable until the exception is handled
  }

  void ExecutionContext::handleSyscall(cpu::ThreadState* ctx)
  {
    ASSERT(&threadState == ctx);
    auto& code = ctx->rdi;
    auto& userctx = ctx->rsi;
    auto portal = ctx->rdx;
    auto kobj = ctx->r10;
    MLOG_DETAIL(mlog::syscall, "handleSyscall", DVAR(this));
    MLOG_DETAIL(mlog::syscall, DVARhex(ctx->rip), DVARhex(ctx->rflags), DVARhex(ctx->rsp));
    MLOG_DETAIL(mlog::syscall, DVARhex(ctx->rdi), DVARhex(ctx->rsi), DVARhex(ctx->rdx),
                     DVARhex(ctx->r10), DVARhex(ctx->r8), DVARhex(ctx->r9));

    switch(SyscallCode(code)) {

      case SYSCALL_EXIT:
        MLOG_INFO(mlog::syscall, "exit");
        setFlag(IS_EXITED);
        break;

      case SYSCALL_POLL:
        MLOG_INFO(mlog::syscall, "poll");
        setFlag(IN_WAIT);
        break;

      case SYSCALL_WAIT:
        MLOG_INFO(mlog::syscall, "wait");
        setFlag(IN_WAIT | IS_WAITING);
        if (!notificationQueue.empty()) clearFlag(IS_WAITING); // because of race with notifier
        break;

      case SYSCALL_INVOKE:
        MLOG_INFO(mlog::syscall, "invoke", DVAR(portal), DVAR(kobj), DVARhex(userctx));
        code = uint64_t(syscallInvoke(CapPtr(portal), CapPtr(kobj), userctx).state());
        break;

      case SYSCALL_INVOKE_POLL:
        MLOG_INFO(mlog::syscall, "invoke_poll", DVAR(portal), DVAR(kobj), DVARhex(userctx));
        code = uint64_t(syscallInvoke(CapPtr(portal), CapPtr(kobj), userctx).state());
        if (Error(code) != Error::SUCCESS) break;
        setFlag(IN_WAIT);
        break;

      case SYSCALL_INVOKE_WAIT:
        MLOG_INFO(mlog::syscall, "invoke_wait", DVAR(portal), DVAR(kobj), DVARhex(userctx));
        code = uint64_t(syscallInvoke(CapPtr(portal), CapPtr(kobj), userctx).state());
        if (Error(code) != Error::SUCCESS) break;
        setFlag(IN_WAIT | IS_WAITING);
        if (!notificationQueue.empty()) clearFlag(IS_WAITING); // because of race with notifier
        break;

      case SYSCALL_DEBUG: {
        MLOG_INFO(mlog::syscall, "debug", DVARhex(userctx), DVAR(portal));
        mlog::Logger<mlog::FilterAny> user("user");
        user.error(mlog::DebugString((char const*)userctx, portal));
        code = uint64_t(Error::SUCCESS);
        break;
      }

      default: break;
    }
    MLOG_DETAIL(mlog::syscall, DVARhex(userctx), DVAR(code));
  }

  optional<void> ExecutionContext::syscallInvoke(CapPtr portal, CapPtr dest, uint64_t user)
  {
    TypedCap<ICapMap> cs(_cs.cap());
    if (!cs) RETHROW(cs);
    TypedCap<IPortal> p(cs.lookup(portal, 32, false));
    if (!p) RETHROW(p);
    RETURN(p.sendInvocation(dest, user));
  }

  void ExecutionContext::resume() {
    if (!isReady()) return;
    if (clearFlag(IN_WAIT) & IN_WAIT) {
      MLOG_DETAIL(mlog::ec, "try to resume from wait state");
      auto e = notificationQueue.pull();
      if (e) {
        auto ev = e->get()->deliver();
        threadState.rsi = ev.user;
        threadState.rdi = ev.state;
      } else {
        threadState.rsi = 0;
        threadState.rdi = uint64_t(Error::NO_NOTIFICATION);
      }
      MLOG_DETAIL(mlog::syscall, DVARhex(threadState.rsi), DVAR(threadState.rdi));
    }

    // remove myself from the last place's current_ec if still there
    std::atomic<ISchedulable*>* thisPlace = current_ec.addr();
    if (lastPlace != nullptr && thisPlace != lastPlace) {
      ISchedulable* self = this;
      lastPlace->compare_exchange_strong(self, nullptr);
    }

    // load own context stuff if someone else was running on this place
    ISchedulable* prev = thisPlace->load();
    if (prev != this) {
      if (prev) prev->unload();
      TypedCap<IPageMap> as(_as);
      if (!as) return (void)setFlag(NO_AS); // might have been revoked concurrently
      cpu::thread_state = &threadState;
      lastPlace = thisPlace;
      thisPlace->store(this);
      auto info = as->getPageMapInfo(as.cap());
      MLOG_INFO(mlog::ec, "load addrspace", DVAR(this), DVARhex(info.table.physint()));
      getLocalPlace().setCR3(info.table);
      /// @todo restore FPU and vector state
    }

    cpu::return_to_user();
  }

  void ExecutionContext::unload()
  {
    /// @todo save FPU and vector state
  }

  optional<void> ExecutionContext::deleteCap(Cap self, IDeleter& del)
  {
    if (self.isOriginal()) {
      if (lastPlace != nullptr) {
        ISchedulable* self = this;
        lastPlace->compare_exchange_strong(self, nullptr);
      }
      _as.reset();
      _cs.reset();
      _sched.reset();
      del.deleteObject(del_handle);
    }
    RETURN(Error::SUCCESS);
  }

  void ExecutionContext::deleteObject(Tasklet* t, IResult<void>* r)
  {
    monitor.doDelete(t, [=](Tasklet* t) { this->memory->free(t, r, this, sizeof(ExecutionContext)); });
  }

  void ExecutionContext::invoke(Tasklet* t, Cap self, IInvocation* msg)
  {
    monitor.request(t, [=](Tasklet* t){
        Error err = Error::NOT_IMPLEMENTED;
        switch (msg->getProtocol()) {
         case protocol::KernelObject::proto:
           err = protocol::KernelObject::dispatchRequest(this, msg->getMethod(), self, msg);
           break;
        case protocol::ExecutionContext::proto:
          err = protocol::ExecutionContext::dispatchRequest(this, msg->getMethod(), t, self, msg);
          break;
        }
        if (err != Error::INHIBIT) {
          msg->replyResponse(err);
          monitor.requestDone();
        }
      } );
  }

  Error ExecutionContext::getDebugInfo(Cap self, IInvocation* msg)
  {
    return writeDebugInfo("ExecutionContext", self, msg);
  }

  optional<ExecutionContext*>
    ExecutionContextFactory::factory(CapEntry* dstEntry, CapEntry* memEntry, Cap memCap,
        IAllocator* mem, message_type* data, IInvocation* msg)
    {
      auto obj = mem->create<ExecutionContext>();
      if (!obj) RETHROW(obj);
      Cap cap(*obj); /// @todo should have EC specific rights
      auto res = cap::inherit(*memEntry, *dstEntry, memCap, cap);
      if (!res) {
        mem->free(*obj); // mem->release(obj) goes through IKernelObject deletion mechanism
        RETHROW(res);
      }
      if (data) {// configure according to message
        ASSERT(implies(data, msg));
        auto regRes = obj->setRegisters(data->regs);
        if (!regRes) RETHROW(regRes);
        if (data->as()) {
          auto res = obj->setAddressSpace(msg->lookupEntry(data->as()));
          if (!res) RETHROW(res);
        }
        if (data->cs()) {
          auto res = obj->setCapSpace(msg->lookupEntry(data->cs()));
          if (!res) RETHROW(res);
        }
        if (data->sched()) {
          auto res = obj->setSchedulingContext(msg->lookupEntry(data->sched()));
          if (!res) RETHROW(res);
        }
        if (data->start) {
          obj->setEntryPoint(data->regs.rip);
        }
      }
      return *obj;
    }

} // namespace mythos


