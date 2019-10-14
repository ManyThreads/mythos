#include "runtime/futex.h"
#include "runtime/mlog.hh"

#include <errno.h>
//#include <atomic>
#include <stdatomic.h>

static int futex_wait(uint32_t *uaddr, unsigned int flags, uint32_t val,
uint32_t *abs_time, uint32_t bitset)
{
	MLOG_DETAIL(mlog::app, DVAR(*uaddr), DVAR(val));
	while(atomic_load(uaddr) == val);
	return 0;
}

static int futex_wake(uint32_t *uaddr, unsigned int flags, int nr_wake, uint32_t bitset){
	MLOG_DETAIL(mlog::app, DVAR(*uaddr));
	atomic_fetch_add(uaddr, 1);
	MLOG_DETAIL(mlog::app, "Futex_wait return" );
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