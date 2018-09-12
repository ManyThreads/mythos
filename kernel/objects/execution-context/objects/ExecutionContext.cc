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
    : memory(memory)
  {
    setFlags(IS_EXITED | NO_AS | NO_SCHED);
    threadState.clear();
    fpuState.clear();
    threadState.rflags = x86::FLAG_IF;
  }

  void ExecutionContext::setFlagsSuspend(flag_t f)
  {
    auto prev = setFlags(f | DONT_PREMP_ON_SUSPEND);
    if (needPreemption(prev) && !isReady()) {
      auto sched = _sched.get();
      if (sched) sched->preempt(&ec_handle);
    }
    // maybe wait for IS_NOT_RUNNING, preemption is not synchronous
  }

  void ExecutionContext::clearFlagsResume(flag_t f)
  {
    auto prev = clearFlags(f);
    MLOG_DETAIL(mlog::ec, "cleared flag", DVAR(this), DVARhex(f), DVARhex(prev), DVARhex(flags.load()), isReady());
    if (isBlocked(prev) && isReady()) {
      auto sched = _sched.get();
      MLOG_DETAIL(mlog::ec, "trying to wake up the scheduler");
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
    clearFlagsResume(NO_AS);
  }

  void ExecutionContext::unbind(optional<IPageMap*>)
  {
    MLOG_INFO(mlog::ec, "setAddressSpace unbind", DVAR(this));
    setFlagsSuspend(NO_AS);
  }

  optional<void> ExecutionContext::setSchedulingContext(optional<CapEntry*> sce)
  {
    MLOG_INFO(mlog::ec, "setScheduler", DVAR(this), DVAR(sce));
    ASSERT(currentPlace.load() == nullptr);
    TypedCap<IScheduler> obj(sce);
    if (!obj) RETHROW(obj);
    RETURN(_sched.set(this, *sce, obj.cap()));
  }

  optional<void> ExecutionContext::setSchedulingContext(
      Tasklet* t, IInvocation* msg, optional<CapEntry*> sce)
  {
    MLOG_INFO(mlog::ec, "setScheduler", DVAR(this), DVAR(sce));
    TypedCap<IScheduler> obj(sce);
    if (!obj) RETHROW(obj);
    
    // check if the EC state is currently loaded somewhere
    auto place = currentPlace.exchange(nullptr);
    if (place != nullptr) {
        // save my state
        place->run(t->set([this, msg, sce](Tasklet*){
            this->fpuState.save();
            TypedCap<IScheduler> obj(sce);
            if (!obj) {
              msg->replyResponse(obj.state());
              monitor.requestDone();
            }
            auto res = _sched.set(this, *sce, obj.cap());
            msg->replyResponse(res);
            monitor.requestDone();
            }));
        RETURN(Error::INHIBIT);
    } else {
        RETURN(_sched.set(this, *sce, obj.cap()));
    }
  }
  
  Error ExecutionContext::unsetSchedulingContext()
  {
    _sched.reset();
    return Error::SUCCESS;
  }
  
  void ExecutionContext::bind(optional<IScheduler*>)
  {
    MLOG_INFO(mlog::ec, "setScheduler bind", DVAR(this));
    clearFlagsResume(NO_SCHED);
  }

  void ExecutionContext::unbind(optional<IScheduler*> sched)
  {
    MLOG_INFO(mlog::ec, "setScheduler unbind", DVAR(this));
    ASSERT(sched);
    setFlags(NO_SCHED);
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
    clearFlagsResume(IS_EXITED);
  }

  optional<CapEntryRef> ExecutionContext::lookupRef(CapPtr ptr, CapPtrDepth ptrDepth, bool writable)
  {
    TypedCap<ICapMap> cs(_cs.cap());
    if (!cs) RETHROW(cs);
    RETURN(cs.lookup(ptr, ptrDepth, writable));
  }

  Error ExecutionContext::invokeConfigure(Tasklet* t, Cap, IInvocation* msg)
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
    else if (data.sched() != null_cap) {
      return setSchedulingContext(t, msg, msg->lookupEntry(data.sched())).state();
    }

    return Error::SUCCESS;
  }

  Error ExecutionContext::invokeReadRegisters(Tasklet* t, Cap, IInvocation* msg)
  {
    this->msg = msg;
    auto const& data = *msg->getMessage()->cast<protocol::ExecutionContext::ReadRegisters>();
    if (data.suspend) setFlags(IS_TRAPPED);
    setFlags(REGISTER_ACCESS);
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
        clearFlagsResume(REGISTER_ACCESS);
        msg->replyResponse(Error::SUCCESS);
        msg = nullptr;
        monitor.responseAndRequestDone();
      } );
  }

  Error ExecutionContext::invokeWriteRegisters(Tasklet* t, Cap, IInvocation* msg)
  {
    this->msg=msg;
    auto const& data = *msg->getMessage()->cast<protocol::ExecutionContext::WriteRegisters>();
    setFlags(REGISTER_ACCESS);
    if (data.resume) clearFlags(IS_TRAPPED);
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
        if (!res) setFlags(IS_TRAPPED);
        clearFlagsResume(REGISTER_ACCESS);
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
      // \TODO don't use PhysPtr because the canonical address belong to the logical addresses
    if (!PhysPtr<void>(fs).canonical() || !PhysPtr<void>(gs).canonical())
      THROW(Error::NON_CANONICAL_ADDRESS);
    threadState.fs_base = fs;
    threadState.gs_base = gs;
    MLOG_DETAIL(mlog::ec, "set fs/gs", DVAR(this), DVARhex(fs), DVARhex(gs));
    
    RETURN(Error::SUCCESS);
  }

  Error ExecutionContext::invokeResume(Tasklet*, Cap, IInvocation*)
  {
    clearFlagsResume(IS_TRAPPED);
    return Error::SUCCESS;
  }

  Error ExecutionContext::invokeSuspend(Tasklet* t, Cap, IInvocation* msg)
  {
    this->msg=msg;
    setFlags(IS_TRAPPED);
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
      setFlagsSuspend(IS_TRAPPED);
      clearFlagsResume(IS_TRAPPED);
    }
    return res.state();
  }

  void ExecutionContext::notify(INotifiable::handle_t* event)
  {
    MLOG_INFO(mlog::ec, "got notification", DVAR(this), DVAR(event));
    notificationQueue.push(event);
    clearFlagsResume(IS_WAITING);
  }

  void ExecutionContext::denotify(INotifiable::handle_t* event)
  {
    notificationQueue.remove(event);
  }

  void ExecutionContext::handleTrap()
  {
    auto ctx = &threadState;
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
    MLOG_ERROR(mlog::ec, "...", DVARhex(ctx->fs_base), DVARhex(ctx->gs_base));
    setFlags(IS_TRAPPED | IS_NOT_RUNNING); // mark as not executable until the exception is handled
  }

  void ExecutionContext::handleInterrupt()
  {
    setFlags(IS_NOT_RUNNING);
  }

  void ExecutionContext::handleSyscall()
  {
    setFlags(IS_NOT_RUNNING);
    auto ctx = &threadState;
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
        setFlags(IS_EXITED);
        break;

      case SYSCALL_POLL:
        MLOG_INFO(mlog::syscall, "poll");
        setFlags(IN_WAIT);
        break;

      case SYSCALL_WAIT: {
        auto prevState = setFlags(IN_WAIT | IS_WAITING);
        MLOG_INFO(mlog::syscall, "wait", DVARhex(prevState));
        if (!notificationQueue.empty() || (prevState & IS_NOTIFIED))
          clearFlags(IS_WAITING); // because of race with notifier
        break;
      }

      case SYSCALL_INVOKE:
        MLOG_INFO(mlog::syscall, "invoke", DVAR(portal), DVAR(kobj), DVARhex(userctx));
        code = uint64_t(syscallInvoke(CapPtr(portal), CapPtr(kobj), userctx).state());
        break;

      case SYSCALL_INVOKE_POLL:
        MLOG_INFO(mlog::syscall, "invoke_poll", DVAR(portal), DVAR(kobj), DVARhex(userctx));
        code = uint64_t(syscallInvoke(CapPtr(portal), CapPtr(kobj), userctx).state());
        if (Error(code) == Error::SUCCESS) setFlags(IN_WAIT); // else return the error code
        break;

      case SYSCALL_INVOKE_WAIT: {
        MLOG_INFO(mlog::syscall, "invoke_wait", DVAR(portal), DVAR(kobj), DVARhex(userctx));
        code = uint64_t(syscallInvoke(CapPtr(portal), CapPtr(kobj), userctx).state());
        if (Error(code) != Error::SUCCESS) break; // just return the error code
        auto prevState = setFlags(IN_WAIT | IS_WAITING);
        if (!notificationQueue.empty() || (prevState & IS_NOTIFIED))
          clearFlags(IS_WAITING); // because of race with notifier
        break;
      }

      case SYSCALL_DEBUG: {
        MLOG_DETAIL(mlog::syscall, "debug", (void*)userctx, portal);
        mlog::Logger<mlog::FilterAny> user("user");
        // userctx => address in users virtual memory. Yes, we fully trust the user :(
        // portal => string length
        mlog::sink->write((char const*)userctx, portal);
        code = uint64_t(Error::SUCCESS);
        break;
      }

      case SYSCALL_NOTIFY: {
        TypedCap<ICapMap> cs(_cs); // .cap()
        if (!cs) { code = uint64_t(cs.state()); break; }
        TypedCap<ISchedulable> th(cs.lookup(CapPtr(portal), 32, false));
        if (!th) { code = uint64_t(th.state()); break; }
        MLOG_INFO(mlog::syscall, "semaphore notify syscall", DVAR(portal), DVAR(th.obj()));
        th->semaphoreNotify();
        code = uint64_t(Error::SUCCESS);
        break;
      }

      default: break;
    }
    MLOG_DETAIL(mlog::syscall, DVARhex(userctx), DVAR(code));
  }

  void ExecutionContext::semaphoreNotify()
  {
    auto prev = setFlags(IS_NOTIFIED);
    MLOG_DETAIL(mlog::syscall, "receiving notify syscall", DVARhex(prev));
    clearFlagsResume(IS_WAITING);
  }

  optional<void> ExecutionContext::syscallInvoke(CapPtr portal, CapPtr dest, uint64_t user)
  {
    TypedCap<ICapMap> cs(_cs.cap());
    if (!cs) RETHROW(cs);
    TypedCap<IPortal> p(cs.lookup(portal, 32, false));
    if (!p) RETHROW(p);
    RETURN(p.sendInvocation(dest, user));
  }

  // this is called by a scheduler on some hardware thread (aka place)
  void ExecutionContext::resume() {
    if (!isReady()) return;
    auto prevState = clearFlags(IN_WAIT);
    MLOG_DETAIL(mlog::ec, "try to resume", DVARhex(prevState));
    if (prevState & IN_WAIT) {
      MLOG_DETAIL(mlog::ec, "try to resume from wait state");
      clearFlags(IS_NOTIFIED); // this is safe because we wake up anyway
      auto e = notificationQueue.pull();
      if (e) {
        auto ev = e->get()->deliver();
        threadState.rsi = ev.user;
        threadState.rdi = ev.state;
      } else {
        threadState.rsi = 0;
        threadState.rdi = uint64_t(Error::NO_MESSAGE);
      }
      //MLOG_DETAIL(mlog::syscall, DVARhex(threadState.rsi), DVAR(threadState.rdi));
    }

    // load own context stuff if someone else was running on this place
    if (cpu::thread_state.get() != &threadState) {
      // the next this->saveState() will be executed at the same hardware thread
      // and therefore can not race with us

      // this should only be called if you are sure that the previous state has been saved
      ASSERT(cpu::thread_state.get() == nullptr);
      auto oldPlace = currentPlace.exchange(&getLocalPlace());
      ASSERT(oldPlace == nullptr || oldPlace == &getLocalPlace());

      cpu::thread_state = &threadState;
      fpuState.restore();

      TypedCap<IPageMap> as(_as);
      // might have been revoked concurrently
      // @todo: setting the flag here might introduce are race with setting a new AS
      // @todo: is the address space really reloaded when it was changed?
      // @todo: this check should be done before loading all the context!
      if (!as) return (void)setFlags(NO_AS);
      auto info = as->getPageMapInfo(as.cap());
      MLOG_DETAIL(mlog::ec, "load addrspace", DVAR(this), DVARhex(info.table.physint()));
      getLocalPlace().setCR3(info.table);
    }

    MLOG_INFO(mlog::syscall, "resuming", DVAR(this), DVARhex(threadState.rip), DVARhex(threadState.rsp));
    clearFlags(IS_NOT_RUNNING); // TODO: check for races
    cpu::return_to_user(); // does never return to here
  }
  
  void ExecutionContext::saveState()
  {
      // can only be called once after resume, 
      // and this still has to be the right execution context
      ASSERT(cpu::thread_state.get() == &threadState);
      cpu::thread_state = nullptr;

      // remember that we saved the fpu state and don't need unloading on migration etc
      auto oldPlace = currentPlace.exchange(nullptr);
      ASSERT(oldPlace == &getLocalPlace());

      fpuState.save();
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
      if (!obj) {
        dstEntry->reset();
        RETHROW(obj);
      }
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


