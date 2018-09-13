# Execution Contexts in MyThOS

This implementation deviates from the object monitors with tasklet-based synchronisation. The invocation handling uses the object monitor. The scheduler that resumes the execution context runs potentially on a different hardware thread and accesses the objects state directly without tasklet  synchronisation. Instead a bitmap with atomic updates is used to synchronise between invocation handling and the hardware thread where the execution context is scheduled. 

## Design Alternatives

A design where the scheduler uses tasklet synchronisation too is possible. This will need extensions in the object monitor to migrate the tasklet processing to the scheduler's hardware thread. Then the thread state is loaded without having to worry about concurrent invocations. Finally, the scheduler tries to release the object monitor. If there are still enqueued tasklets, the switch to user mode fails and the kernel proceeds to process these tasklets.

Comparing the complexity and performance of both approaches should be interesting.


If the execution context's state, that is its register contents and FPU state, is stored in a frame, migrating threads can be achieved withouth migrating execution context kernel objects. Each execution context is created for a specific hardware thread and never moves. Instead the EC is supended, then the frame with the stored state is attached to another EC and this EC is resumed. Some of the state flags, e.g. IN_WAIT, IS_WAITING, IS_NOTIFIED need to be moved to the state stored in the frame in order to become migratable. The advantage of this design is, that all invocations and accesses can be directed to the EC's home, which significantly reduces synchronisation challenges. On the downside, the interruptions increase, but they would affect the execution context and hardware thread that they involve anyway. Some of the registers, e.g. rflags, need to be overridden and loaded from outside the frame in the kernel exit code. Otherwise, the user mode can disable interrupts and similar things.


## Life Cycle States


### Ready State: multiple blocking flags

The exection context is READY (unblocked) when it is not BLOCKED.
It is BLOCKED when at least one of the following flags is set:
* IS_WAITING: the application waits for a notification
* IS_TRAPPED: the application trapped
* IS_EXITED: the application returned via the exit system call
* NO_AS: the address space PageMap level 4
* NO_SCHED: the scheduling context / scheduler / hardware thread
* REGISTER_ACCESS: someone requested to read the register contents

This helps to keep seperate the multiple reasons that prevent the execution thread from running. Otherwise, we would need to check all state variables, e.g. pointers, seperately whenever we change one of them.


### Loaded State: NOT_LOADED

The execution context's state needs to be loaded before switching to user mode. It needs to be saved before loading another execution context's state. The state is LOADED when all of the following conditions are met:
* mythos::current_ec == ec at the cpu. It points to the loaded execution context. This is used by the system call and trap handling to forward to the correct object.
* cpu::thread_state == ec->thread_state at the cpu. It points to the register struct used during kernel entry via system calls, traps, and interrupts.
* ec->current_place == &getLocalPlace() at the cpu. It points to the hardware thread where the execution context's state is loaded.
* ec->fpu_state is loaded at the cpu. This is needed for user mode floating point instructions.
* NOT_LOADED flag is cleared.

TODO might load AS. however, the current interface checks anyway if the pointer changed. thus reloading it in resume() is good enough for the time being.


### Running State: DONT_PREEMPT, NOT_RUNNING

There is a difference between the hardware thread actually being in user mode and other hardware threads thinking that it already or still is. The reason is, that there is a time gap between entering the kernel, acquiring the tasklet queue to not receive IPIs, and the execution context knowing that it left user mode.

When blocking the execution context, is it necessary to preempt the hardware thread? This is answered by the DONT_PREEMPT flag. Before switching to user mode, the execution context clears its DONT_PREEMPT flag. When blocking the execution context we can also atomically check and set the DONT_PREEMPT flag. If it was not set, sending a preemption IPI is necessary, which is achieved by acquiring the hardware thread's task queue. It is sufficient that only the first sends the preemption.

Some code might want to wait until the execution context actually returned from user mode. This is anwered by the NOT_RUNNING flag. Before switching to user mode, the execution context clears its NOT_RUNNING flag. After entry into the kernel, the NOT_RUNNING and the DONT_PREEMPT flags are set again. Code can spin lock on the NOT_RUNNING flag until it is set. However, it is necessary to also check DONT_PREEMPT and send an preemption if it is not set. 

TODO It is also necessary to process high priority tasklets during the spin lock?


### Single Binary Semaphore: IS_NOTIFIED, IS_WAITING, IN_WAIT

The IS_NOTIFIED flag implements a single binary semaphore. It is used as foundation for all other semaphores, for example the notification from portals and interrupts. In order to prevent lost updates and forwaring the notification correctly to user mode, two additional flags are used.

IS_WAITING blocks the threads execution until IS_NOTIFIED is set. After setting IS_WAITING or IS_NOTIFIED, it is checked if both are now set. If this is the case, both are cleared, which makes the execution context runnable. 

Seperately, IN_WAIT is used to remember that the execution context exited user mode with a wait() system call. This means that the kernel has to transmit a return value to the user mode. It is cleared when resuming and transmits the notification to user mode.

TODO change to NOT_NOTIFIED simplifies the use of atomic clearFlags() and setFlags()?


### Transition Types

* setFlagsSuspend(flags): atomically set at least one of the blocking flags and DONT_PREEMPT. If DONT_PREEMPT was not set before this, then the hardware thread ec->current_place needs to be preempted.
* clearFlagsResume(flags): atomically clear at least one of the blocking flags. Inform the scheduler if the execution context was blocked but became unblocked.
* loadState(): load the execution context's state 
* resume(): check that the execution context is loaded at this hardware thread. Atomically clear NOT_RUNNING and DONT_PREEMPT and check if the execution context was ready. If it was BLOCKED instead, then set both flags again and return. Otherwise load the address space and return to user mode.


