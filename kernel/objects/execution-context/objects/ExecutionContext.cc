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
#include "async/SynchronousTask.hh"
#include "objects/mlog.hh"
#include "objects/ops.hh"
#include "objects/DebugMessage.hh"
#include "objects/IPageMap.hh"
#include "util/error-trace.hh"
#include "mythos/syscall.hh"

namespace mythos {

  ExecutionContext::ExecutionContext(IAsyncFree* memory)
    : flags(0)
    , memory(memory)
  {
    setFlags(IS_TRAPPED + NO_AS + NO_SCHED
        + DONT_PREEMPT + NOT_LOADED + NOT_RUNNING);
    threadState.clear();
    threadState.rflags = x86::FLAG_IF; // ensure that interrupts are enabled in user mode
    fpuState.clear();
  }

  void ExecutionContext::setFlagsSuspend(flag_t f)
  {
    auto prev = setFlags(f | DONT_PREEMPT);
    MLOG_DETAIL(mlog::ec, "set flag", DVAR(this), DVARhex(f), DVARhex(prev),
                DVARhex(flags.load()), isReady());
    if (needPreemption(prev) && !isReady()) {
        auto place = currentPlace.load();
        /// @todo is place->preempt() sufficient without waiting?
        if (place) synchronousAt(place) << [this]() {
            MLOG_DETAIL(mlog::ec, "suspended", DVAR(this));
        };
    }
  }

  void ExecutionContext::clearFlagsResume(flag_t f)
  {
    auto prev = clearFlags(f);
    MLOG_DETAIL(mlog::ec, "cleared flag", DVAR(this), DVARhex(f), DVARhex(prev),
                DVARhex(flags.load()), isReady());
    if (isBlocked(prev) && isReady()) {
      auto sched = _sched.get();
      MLOG_DETAIL(mlog::ec, "inform the scheduler about becoming ready");
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
    setFlagsSuspend(NO_SCHED);
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
    setFlags(REGISTER_ACCESS | DONT_PREEMPT | (data.suspend?IS_TRAPPED:0));

    auto home = currentPlace.load();
    if (home != nullptr) {
        MLOG_DETAIL(mlog::ec, "send preemption message", DVAR(this), DVAR(home));
        home->run(t->set([this](Tasklet* t){
            this->readSleepResponse.response(t, optional<void>(Error::SUCCESS));
        }));
    } else readThreadRegisters(t, optional<void>(Error::SUCCESS));
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

    auto home = currentPlace.load();
    if (home != nullptr) {
        MLOG_DETAIL(mlog::ec, "send preemption message", DVAR(this), DVAR(home));
        home->run(t->set([this](Tasklet* t){
            this->writeSleepResponse.response(t, optional<void>(Error::SUCCESS));
        }));
    } else writeThreadRegisters(t, optional<void>(Error::SUCCESS));
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

  void ExecutionContext::setTrapped(bool val)
  {
    if (val) setFlagsSuspend(IS_TRAPPED);
    else clearFlagsResume(IS_TRAPPED);
  }

  Error ExecutionContext::invokeSuspend(Tasklet* t, Cap, IInvocation* msg)
  {
    this->msg = msg;
    setFlags(IS_TRAPPED + DONT_PREEMPT);

    auto home = currentPlace.load();
    if (home != nullptr) {
        MLOG_DETAIL(mlog::ec, "send preemption message", DVAR(this), DVAR(home));
        home->run(t->set([this](Tasklet* t){
            this->sleepResponse.response(t, optional<void>(Error::SUCCESS));
        }));
    } else suspendThread(t, optional<void>(Error::SUCCESS));
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
      // TODO is FS snd GS saved on suspend? Then async syscall will be overwritten by the suspend code!
      setFlagsSuspend(IS_TRAPPED);
      clearFlagsResume(IS_TRAPPED);
    }
    return res.state();
  }

  void ExecutionContext::attachKEvent(IKEventSink::handle_t* event)
  {
    MLOG_INFO(mlog::ec, "got KEvent", DVAR(this), DVAR(event));
    eventQueue.push(event);
    clearFlagsResume(IS_WAITING);
  }

  void ExecutionContext::detachKEvent(IKEventSink::handle_t* event)
  {
    eventQueue.remove(event);
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
    setFlags(IS_TRAPPED | NOT_RUNNING); // mark as not executable until the exception is handled
  }

  void ExecutionContext::handleSyscall()
  {
    setFlags(NOT_RUNNING);
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
        setFlags(IS_TRAPPED);
        break;

      case SYSCALL_POLL:
        MLOG_INFO(mlog::syscall, "poll");
        setFlags(IN_WAIT);
        break;

      case SYSCALL_WAIT: {
        auto prevState = setFlags(IN_WAIT | IS_WAITING);
        //MLOG_WARN(mlog::syscall, "wait", DVARhex(prevState));
        if (!eventQueue.empty() || (prevState & IS_NOTIFIED)) {
          //MLOG_WARN(mlog::syscall, "skip wait");
          clearFlags(IS_WAITING); // because of race with KEventSource
        }
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
        if (!eventQueue.empty() || (prevState & IS_NOTIFIED))
          clearFlags(IS_WAITING); // because of race with KEventSource
        break;
      }

      case SYSCALL_DEBUG: {
        MLOG_DETAIL(mlog::syscall, "debug", (void*)userctx, portal);
        mlog::Logger<mlog::FilterAny> user("user");
        // userctx => address in users virtual memory. Yes, we fully trust the user :(
        // portal => string length
	char str[300];
	memcpy(&str[0], reinterpret_cast<char*>(userctx), portal<300?portal:300);
        mlog::sink->write((char const*)&str[0], portal);
        code = uint64_t(Error::SUCCESS);
        break;
      }

      case SYSCALL_SIGNAL: {
        TypedCap<ICapMap> cs(_cs); // .cap()
        if (!cs) { code = uint64_t(cs.state()); break; }
        TypedCap<ISignalable> th(cs.lookup(CapPtr(portal), 32, false));
        if (!th) { code = uint64_t(th.state()); break; }
        MLOG_INFO(mlog::syscall, "semaphore signal syscall", DVAR(portal), DVAR(th.obj()));
        th->signal(th.cap().data());
        code = uint64_t(Error::SUCCESS);
        break;
      }

      default: break;
    }
    MLOG_DETAIL(mlog::syscall, DVARhex(userctx), DVAR(code));
  }

  optional<void> ExecutionContext::signal(CapData data)
  {
    auto prev = setFlags(IS_NOTIFIED);
    MLOG_DETAIL(mlog::syscall, "receiving signal", DVAR(data), DVARhex(prev));
    clearFlagsResume(IS_WAITING);
    RETURN(Error::SUCCESS);
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
        // This is called by a scheduler on some hardware thread (aka place).

        // Atomically clear NOT_RUNNING and DONT_PREEMPT and check whether
        // the execution context is (still) ready and loaded at this hardware thread.
        // If it was not ready, then set both flags again and return.
        auto prev = flags.load();
        while (true) {
            if (isBlocked(prev)) {
                MLOG_DETAIL(mlog::ec, "resume failed because blocked",
                            DVAR(this), DVARhex(prev));
               return;
            }
            flag_t newflags = prev & flag_t(~(NOT_RUNNING + DONT_PREEMPT));
            if (flags.compare_exchange_weak(prev, newflags)) break;
            // otherwise repeat with new prev
        }
        ASSERT(!(prev & NOT_LOADED));
        ASSERT(currentPlace.load() == &getLocalPlace());

        // return one KEvent to the user mode if it was waiting for some
        auto prevWait = clearFlags(IN_WAIT);
        if (prevWait & IN_WAIT) {
            // clear IS_NOTIFIED only if the user mode was waiting for it
            // we won't clear it twice without a second wait() system call
            clearFlags(IS_NOTIFIED); 

            // return a KEvent if any
            auto e = eventQueue.pull();
            if (e) {
                auto ev = e->get()->deliverKEvent();
                threadState.rsi = ev.user;
                threadState.rdi = ev.state;
            } else {
                threadState.rsi = 0;
                threadState.rdi = uint64_t(Error::NO_MESSAGE);
            }
            MLOG_DETAIL(mlog::ec, DVAR(this), "return one KEvent", 
                        DVARhex(threadState.rsi), DVAR(threadState.rdi));
        }

        // Reload the address space if it has changed.
        // It might have been revoked concurrently since we checked the blocking flags.
        // In that case we abort the resume. The revoke will update NO_AS for us.
        {
            TypedCap<IPageMap> as(_as);
            if (!as) return;
            auto info = as->getPageMapInfo(as.cap());
            MLOG_DETAIL(mlog::ec, "load addrspace", DVAR(this), DVARhex(info.table.physint()));
            getLocalPlace().setCR3(info.table); // without reload if not changed
        }

        MLOG_DETAIL(mlog::syscall, "resuming", DVAR(this),
                    DVARhex(threadState.rip), DVARhex(threadState.rsp));
        cpu::return_to_user(); // does never return
    }

    void ExecutionContext::loadState()
    {
        // the assertions are quite redundant, just to be safe during debugging

        // remember where the state is loaded in case the scheduler does not know,
        // the state should not be loaded somewhere else.
        // WARNING this has the same meaning as !NOT_LOADED
        auto oldPlace = currentPlace.exchange(&getLocalPlace());
        ASSERT(oldPlace == nullptr);

        // only one hardware thread can load the execution context, this atomic detects races
        auto prev = clearFlags(NOT_LOADED);
        ASSERT(prev & NOT_LOADED);
        // load registers and fpu, no execution context must be loaded here already
        ASSERT(cpu::thread_state.get() == nullptr);
        cpu::thread_state = &threadState;
        fpuState.restore();
        // tell the kernel that this execution context is in charge now
        // and check that no other was loaded.
        ASSERT(current_ec->load() == nullptr);
        current_ec->store(this);
    }

    void ExecutionContext::saveState()
    {
        // the assertions are quite redundant, just to be safe during debugging

        // can be called only after loadState on the same hardware thread
        auto prev = setFlags(NOT_LOADED);
        ASSERT(!(prev & NOT_LOADED));

        auto oldPlace = currentPlace.exchange(nullptr);
        ASSERT(oldPlace == &getLocalPlace());

        ASSERT(cpu::thread_state.get() == &threadState);
        cpu::thread_state = nullptr;
        fpuState.save();

        // tell the kernel that nobody is in charge now
        ASSERT(current_ec->load() == this);
        current_ec->store(nullptr);
    }

    optional<void> ExecutionContext::deleteCap(CapEntry&, Cap self, IDeleter& del)
    {
        if (self.isOriginal()) { // the object gets deleted, not a capability reference
            setFlags(NO_AS + NO_SCHED); // ensure that the EC looks blocked
            // synchronously preempt and unload the own state
            auto place = currentPlace.load();
            if (place) synchronousAt(place) << [this]() {
                this->saveState();
                // reset scheduler at this point to prevent a race condition with resuming EC
                this->_sched.reset();
                MLOG_DETAIL(mlog::ec, "preempted and unloaded state", DVAR(this));
            };
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
      auto res = cap::inherit(*memEntry, memCap, *dstEntry, cap);
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
        obj->setEntryPoint(data->regs.rip);
        if (data->start) obj->setTrapped(false);
      }
      return *obj;
    }

} // namespace mythos


