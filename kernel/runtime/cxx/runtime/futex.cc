#include "runtime/futex.hh"
#include "runtime/mlog.hh"
#include "runtime/ExecutionContext.hh"
#include "util/TidexMutex.hh"

#include <errno.h>
#include <atomic>

struct FutexQueueElem;

struct FutexQueueElem{
	mythos::CapPtr ec;
	uint32_t* uaddr;
	FutexQueueElem* next;	
}; 

FutexQueueElem* queueHead = nullptr;
FutexQueueElem** queueTail = &queueHead;

struct MuslThreadContext {
	typedef uint32_t ThreadID;
	static ThreadID getThreadID() { return uint32_t(mythos::localEC); }
	static void pollpause(){};
};

typedef mythos::TidexMutex<MuslThreadContext> SpinLock; 
SpinLock readLock;

static int futex_wait(uint32_t *uaddr, unsigned int flags, uint32_t val,
uint32_t *abs_time, uint32_t bitset)
{
	MLOG_DETAIL(mlog::app, DVARhex(uaddr), DVAR(*uaddr), DVAR(val));
	FutexQueueElem qe;
	qe.ec = mythos::localEC;
	qe.uaddr = uaddr;
	qe.next = nullptr;

	volatile uint32_t* addr = uaddr;
	{ 
		SpinLock::Lock guard(readLock);
		*queueTail = &qe;
		queueTail = &qe.next;
	}

	if(val == *addr){
		MLOG_DETAIL(mlog::app, "going to wait");
		auto sysret = mythos::syscall_wait();	
	}

	if(reinterpret_cast<uint64_t>(qe.next) != 1ul){
		MLOG_DETAIL(mlog::app, "next != -1 -> remove this element from queue");
		SpinLock::Lock guard(readLock);
		auto curr = &queueHead;
		while(*curr){
			if((*curr)->next == &qe) break;
			curr = &(*curr)->next;
		}
		
		if(*curr){
			if(queueTail == &(*curr)->next){
				MLOG_DETAIL(mlog::app, "last elem in queue");
				queueTail = curr;
			}
			(*curr)->next = (*curr)->next->next;
		}

	
	}

	MLOG_DETAIL(mlog::app, "Return from futex_wait");
	return 0;
}

static int futex_wake(uint32_t *uaddr, unsigned int flags, int nr_wake, uint32_t bitset){
	MLOG_DETAIL(mlog::app, "Futex_wake" );
	MLOG_DETAIL(mlog::app, DVARhex(uaddr), DVAR(*uaddr));
	
	SpinLock::Lock guard(readLock);
	
	auto curr = &queueHead; 
	for(int i = 0; i < nr_wake; i++){
		while((*curr) && (*curr)->uaddr != uaddr){
			curr = &(*curr)->next;
		}

		if(*curr){
			MLOG_DETAIL(mlog::app, "Found entry" );
			auto entry = *curr;
			*curr = entry->next;
			entry->next = reinterpret_cast<FutexQueueElem*>(1ul);
			MLOG_DETAIL(mlog::app, "Wake EC" );
			mythos::syscall_signal(entry->ec);
		}else{
			MLOG_DETAIL(mlog::app, "Reached end of queue" );
	       		return 0;
		}
	}

	return 0;
}

long do_futex(uint32_t *uaddr, int op, uint32_t val, uint32_t *timeout,
		uint32_t *uaddr2, uint32_t val2, uint32_t val3)
{
	int cmd = op & FUTEX_CMD_MASK;
	unsigned int flags = 0;

	//if (!(op & FUTEX_PRIVATE_FLAG))
		//flags |= FLAGS_SHARED;

	if (op & FUTEX_CLOCK_REALTIME) {
		flags |= FLAGS_CLOCKRT;
		if (cmd != FUTEX_WAIT && cmd != FUTEX_WAIT_BITSET && \
		    cmd != FUTEX_WAIT_REQUEUE_PI)
			return -ENOSYS;
	}

	/*switch (cmd) {
	case FUTEX_LOCK_PI:
	case FUTEX_UNLOCK_PI:
	case FUTEX_TRYLOCK_PI:
	case FUTEX_WAIT_REQUEUE_PI:
	case FUTEX_CMP_REQUEUE_PI:
		if (!futex_cmpxchg_enabled)
			return -ENOSYS;
	}*/

	switch (cmd) {
	case FUTEX_WAIT:
		MLOG_DETAIL(mlog::app, "FUTEX_WAIT");
		val3 = FUTEX_BITSET_MATCH_ANY;
		/* fall through */
	case FUTEX_WAIT_BITSET:	
		MLOG_DETAIL(mlog::app, "FUTEX_WAIT_BITSET");
		return futex_wait(uaddr, flags, val, timeout, val3);
	case FUTEX_WAKE:
		MLOG_DETAIL(mlog::app, "FUTEX_WAKE");
		val3 = FUTEX_BITSET_MATCH_ANY;
		/* fall through */
	case FUTEX_WAKE_BITSET:
		MLOG_DETAIL(mlog::app, "FUTEX_WAKE_BITSET");
		return futex_wake(uaddr, flags, val, val3);
	case FUTEX_REQUEUE:
		MLOG_DETAIL(mlog::app, "FUTEX_REQUEUE");
		return -ENOSYS;//futex_requeue(uaddr, flags, uaddr2, val, val2, NULL, 0);
	case FUTEX_CMP_REQUEUE:
		MLOG_DETAIL(mlog::app, "FUTEX_CMP_REQUEUE");
		return -ENOSYS;//futex_requeue(uaddr, flags, uaddr2, val, val2, &val3, 0);
	case FUTEX_WAKE_OP:
		MLOG_DETAIL(mlog::app, "FUTEX_WAKE_OP");
		return -ENOSYS;//futex_wake_op(uaddr, flags, uaddr2, val, val2, val3);
	case FUTEX_LOCK_PI:
		MLOG_DETAIL(mlog::app, "FUTEX_LOCK_PI");
		return -ENOSYS;//futex_lock_pi(uaddr, flags, timeout, 0);
	case FUTEX_UNLOCK_PI:
		MLOG_DETAIL(mlog::app, "FUTEX_UNLOCK_PI");
		return -ENOSYS;//futex_unlock_pi(uaddr, flags);
	case FUTEX_TRYLOCK_PI:
		MLOG_DETAIL(mlog::app, "FUTEX_TRYLOCK_PI");
		return -ENOSYS;//futex_lock_pi(uaddr, flags, NULL, 1);
	case FUTEX_WAIT_REQUEUE_PI:
		val3 = FUTEX_BITSET_MATCH_ANY;
		MLOG_DETAIL(mlog::app, "FUTEX_WAIT_REQUEUE_PI");
		return -ENOSYS;//futex_wait_requeue_pi(uaddr, flags, val, timeout, val3, uaddr2);
	case FUTEX_CMP_REQUEUE_PI:
		MLOG_DETAIL(mlog::app, "FUTEX_CMP_REQUEUE_PI");
		return -ENOSYS;//futex_requeue(uaddr, flags, uaddr2, val, val2, &val3, 1);
	}
	return -ENOSYS;
}
