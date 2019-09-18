/* -*- mode:C++; -*- */
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
 * Copyright 2014 Randolf Rotta, Maik Krüger, and contributors, BTU Cottbus-Senftenberg 
 */

#include "runtime/mlog.hh"

#include <pthread.h>

//#define use_pthreads_stubs

extern "C" int pthread_create(pthread_t *res, const pthread_attr_t *attrp, void *(*entry)(void *), void *arg){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}

extern "C" int pthread_detach(pthread_t){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}

extern "C" _Noreturn void pthread_exit(void *){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	while(1);
}

extern "C" int pthread_join(pthread_t, void **){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}

#ifdef use_pthreads_stubs
extern "C" pthread_t pthread_self(void){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
}
#endif


extern "C" int pthread_equal(pthread_t, pthread_t){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}


extern "C" int pthread_setcancelstate(int, int *){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}

extern "C" int pthread_setcanceltype(int, int *){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}

extern "C" void pthread_testcancel(void){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
}

extern "C" int pthread_cancel(pthread_t){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}


extern "C" int pthread_getschedparam(pthread_t, int *__restrict, struct sched_param *__restrict){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}

extern "C" int pthread_setschedparam(pthread_t, int, const struct sched_param *){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}

extern "C" int pthread_setschedprio(pthread_t, int){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}


#ifdef use_pthreads_stubs
extern "C" int pthread_once(pthread_once_t *once_control, void (*init_routine)(void)){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__, DVAR(once_control), DVAR(init_routine));
	return 0;
}
#endif


extern "C" int pthread_mutex_init(pthread_mutex_t *__restrict, const pthread_mutexattr_t *__restrict){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}

extern "C" int pthread_mutex_lock(pthread_mutex_t *){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}

extern "C" int pthread_mutex_unlock(pthread_mutex_t *){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}

extern "C" int pthread_mutex_trylock(pthread_mutex_t *){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}

extern "C" int pthread_mutex_timedlock(pthread_mutex_t *__restrict, const struct timespec *__restrict){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}

extern "C" int pthread_mutex_destroy(pthread_mutex_t *){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}

extern "C" int pthread_mutex_consistent(pthread_mutex_t *){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}


extern "C" int pthread_mutex_getprioceiling(const pthread_mutex_t *__restrict, int *__restrict){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}

extern "C" int pthread_mutex_setprioceiling(pthread_mutex_t *__restrict, int, int *__restrict){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}


extern "C" int pthread_cond_init(pthread_cond_t *__restrict, const pthread_condattr_t *__restrict){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}

extern "C" int pthread_cond_destroy(pthread_cond_t *){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}

extern "C" int pthread_cond_wait(pthread_cond_t *__restrict, pthread_mutex_t *__restrict){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}

extern "C" int pthread_cond_timedwait(pthread_cond_t *__restrict, pthread_mutex_t *__restrict, const struct timespec *__restrict){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}

extern "C" int pthread_cond_broadcast(pthread_cond_t *){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}

extern "C" int pthread_cond_signal(pthread_cond_t *){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}


extern "C" int pthread_rwlock_init(pthread_rwlock_t *__restrict, const pthread_rwlockattr_t *__restrict){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}

extern "C" int pthread_rwlock_destroy(pthread_rwlock_t *){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}

extern "C" int pthread_rwlock_rdlock(pthread_rwlock_t *){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}

extern "C" int pthread_rwlock_tryrdlock(pthread_rwlock_t *){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}

extern "C" int pthread_rwlock_timedrdlock(pthread_rwlock_t *__restrict, const struct timespec *__restrict){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}

#ifdef use_pthreads_stubs
extern "C" int pthread_rwlock_wrlock(pthread_rwlock_t *){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}
#endif

#ifdef use_pthreads_stubs
extern "C" int pthread_rwlock_trywrlock(pthread_rwlock_t *){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}
#endif

#ifdef use_pthreads_stubs
extern "C" int pthread_rwlock_timedwrlock(pthread_rwlock_t *__restrict, const struct timespec *__restrict){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}
#endif

#ifdef use_pthreads_stubs
extern "C" int pthread_rwlock_unlock(pthread_rwlock_t *){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}
#endif


extern "C" int pthread_spin_init(pthread_spinlock_t *, int){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}

extern "C" int pthread_spin_destroy(pthread_spinlock_t *){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}

extern "C" int pthread_spin_lock(pthread_spinlock_t *){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}

extern "C" int pthread_spin_trylock(pthread_spinlock_t *){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}

extern "C" int pthread_spin_unlock(pthread_spinlock_t *){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}


extern "C" int pthread_barrier_init(pthread_barrier_t *__restrict, const pthread_barrierattr_t *__restrict, unsigned){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}

extern "C" int pthread_barrier_destroy(pthread_barrier_t *){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}

extern "C" int pthread_barrier_wait(pthread_barrier_t *){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}


#ifdef use_pthreads_stubs
extern "C" int pthread_key_create(pthread_key_t *, void (*)(void *)){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}
#endif

#ifdef use_pthreads_stubs
extern "C" int pthread_key_delete(pthread_key_t){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}
#endif

#ifdef use_pthreads_stubs
extern "C" void *pthread_getspecific(pthread_key_t key){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__, DVAR(key));
	return 0;
}
#endif

#ifdef use_pthreads_stubs
extern "C" int pthread_setspecific(pthread_key_t key, const void *){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__, DVAR(key));
	return 0;
}
#endif


extern "C" int pthread_attr_init(pthread_attr_t *){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}

extern "C" int pthread_attr_destroy(pthread_attr_t *){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}


extern "C" int pthread_attr_getguardsize(const pthread_attr_t *__restrict, size_t *__restrict){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}

extern "C" int pthread_attr_setguardsize(pthread_attr_t *, size_t){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}

extern "C" int pthread_attr_getstacksize(const pthread_attr_t *__restrict, size_t *__restrict){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}

extern "C" int pthread_attr_setstacksize(pthread_attr_t *, size_t){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}

extern "C" int pthread_attr_getdetachstate(const pthread_attr_t *, int *){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}

extern "C" int pthread_attr_setdetachstate(pthread_attr_t *, int){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}

extern "C" int pthread_attr_getstack(const pthread_attr_t *__restrict, void **__restrict, size_t *__restrict){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}

extern "C" int pthread_attr_setstack(pthread_attr_t *, void *, size_t){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}

extern "C" int pthread_attr_getscope(const pthread_attr_t *__restrict, int *__restrict){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}

extern "C" int pthread_attr_setscope(pthread_attr_t *, int){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}

extern "C" int pthread_attr_getschedpolicy(const pthread_attr_t *__restrict, int *__restrict){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}

extern "C" int pthread_attr_setschedpolicy(pthread_attr_t *, int){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}

extern "C" int pthread_attr_getschedparam(const pthread_attr_t *__restrict, struct sched_param *__restrict){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}

extern "C" int pthread_attr_setschedparam(pthread_attr_t *__restrict, const struct sched_param *__restrict){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}

extern "C" int pthread_attr_getinheritsched(const pthread_attr_t *__restrict, int *__restrict){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}

extern "C" int pthread_attr_setinheritsched(pthread_attr_t *, int){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}


extern "C" int pthread_mutexattr_destroy(pthread_mutexattr_t *){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}

extern "C" int pthread_mutexattr_getprioceiling(const pthread_mutexattr_t *__restrict, int *__restrict){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}

extern "C" int pthread_mutexattr_getprotocol(const pthread_mutexattr_t *__restrict, int *__restrict){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}

extern "C" int pthread_mutexattr_getpshared(const pthread_mutexattr_t *__restrict, int *__restrict){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}

extern "C" int pthread_mutexattr_getrobust(const pthread_mutexattr_t *__restrict, int *__restrict){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}

extern "C" int pthread_mutexattr_gettype(const pthread_mutexattr_t *__restrict, int *__restrict){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}

extern "C" int pthread_mutexattr_init(pthread_mutexattr_t *){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}

extern "C" int pthread_mutexattr_setprioceiling(pthread_mutexattr_t *, int){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}

extern "C" int pthread_mutexattr_setprotocol(pthread_mutexattr_t *, int){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}

extern "C" int pthread_mutexattr_setpshared(pthread_mutexattr_t *, int){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}

extern "C" int pthread_mutexattr_setrobust(pthread_mutexattr_t *, int){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}

extern "C" int pthread_mutexattr_settype(pthread_mutexattr_t *, int){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}


extern "C" int pthread_condattr_init(pthread_condattr_t *){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}

extern "C" int pthread_condattr_destroy(pthread_condattr_t *){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}

extern "C" int pthread_condattr_setclock(pthread_condattr_t *, clockid_t){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}

extern "C" int pthread_condattr_setpshared(pthread_condattr_t *, int){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}

extern "C" int pthread_condattr_getclock(const pthread_condattr_t *__restrict, clockid_t *__restrict){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}

extern "C" int pthread_condattr_getpshared(const pthread_condattr_t *__restrict, int *__restrict){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}


extern "C" int pthread_rwlockattr_init(pthread_rwlockattr_t *){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}

extern "C" int pthread_rwlockattr_destroy(pthread_rwlockattr_t *){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}

extern "C" int pthread_rwlockattr_setpshared(pthread_rwlockattr_t *, int){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}

extern "C" int pthread_rwlockattr_getpshared(const pthread_rwlockattr_t *__restrict, int *__restrict){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}


extern "C" int pthread_barrierattr_destroy(pthread_barrierattr_t *){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}

extern "C" int pthread_barrierattr_getpshared(const pthread_barrierattr_t *__restrict, int *__restrict){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}

extern "C" int pthread_barrierattr_init(pthread_barrierattr_t *){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}

extern "C" int pthread_barrierattr_setpshared(pthread_barrierattr_t *, int){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}


extern "C" int pthread_atfork(void (*)(void), void (*)(void), void (*)(void)){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}


extern "C" int pthread_getconcurrency(void){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}

extern "C" int pthread_setconcurrency(int){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}


extern "C" int pthread_getcpuclockid(pthread_t, clockid_t *){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}


#ifdef use_pthreads_stubs
extern "C" void _pthread_cleanup_push(struct __ptcb *, void (*)(void *), void *){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
}
#endif

#ifdef use_pthreads_stubs
extern "C" void _pthread_cleanup_pop(struct __ptcb *, int){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
}
#endif


#ifdef _GNU_SOURCE
extern "C" int pthread_getaffinity_np(pthread_t, size_t, struct cpu_set_t *){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}

extern "C" int pthread_setaffinity_np(pthread_t, size_t, const struct cpu_set_t *){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}

extern "C" int pthread_getattr_np(pthread_t, pthread_attr_t *){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}

extern "C" int pthread_setname_np(pthread_t, const char *){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}

extern "C" int pthread_getattr_default_np(pthread_attr_t *){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}

extern "C" int pthread_setattr_default_np(const pthread_attr_t *){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}

extern "C" int pthread_tryjoin_np(pthread_t, void **){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}

extern "C" int pthread_timedjoin_np(pthread_t, void **, const struct timespec *){
	MLOG_DETAIL(mlog::app, __PRETTY_FUNCTION__);
	return 0;
}

#endif

