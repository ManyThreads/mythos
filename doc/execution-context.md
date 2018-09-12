# Execution Contexts in MyThOS

## Life Cycle and Concurrency Management

### Runnable State

The exection context is READY when it is not BLOCKED.
It is BLOCKED when at least one of the following flags is set:
* IS_WAITING
* IS_TRAPPED
* IS_EXITED
* NO_AS
* NO_SCHED
* IS_EXITED
* REGISTER_ACCESS

### Loaded State

The execution context's state needs to be loaded before switching to user mode. It needs to be saved before loading another execution context's state. The state is LOADED when all of the following conditions are met:
* mythos::current_ec == ec at the cpu. It points to the loaded execution context. This is used by the system call and trap handling to forward to the correct object.
* cpu::thread_state == ec->thread_state at the cpu. It points to the register struct used during kernel entry via system calls, traps, and interrupts.
* ec->current_place == &getLocalPlace() at the cpu. It points to the hardware thread where the execution context's state is loaded.
* 
