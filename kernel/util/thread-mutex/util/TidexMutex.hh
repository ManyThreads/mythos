/* -*- mode:C++; -*- */
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
 * Copyright 2014 Philipp Gypser, Randolf Rotta, BTU Cottbus-Senftenberg
 */
#pragma once

#include <atomic>
#include "cpu/hwthread_pause.hh"
#include "cpu/hwthreadid.hh"

namespace mythos {

  /** Mutex implementation inspired by Tidex Mutex
   * http://concurrencyfreaks.blogspot.de/2014/12/tidex-mutex.html 
   */
  class TidexMutex
  {
  private:
    enum {
      CACHE_LINE_SIZE = 64,
      INVALID_THREAD_ID = 0xFF00
    };

    typedef cpu::ThreadID ThreadID;

    std::atomic<ThreadID> ingress; //< id of the last acquiring thread
    std::atomic<ThreadID> egress; //< id of the last thread that released the mutex

  public: 
    TidexMutex()
      : ingress(INVALID_THREAD_ID)
      , egress(INVALID_THREAD_ID)
    {}
    TidexMutex(TidexMutex const&) = delete;
    void operator=(TidexMutex const&) = delete;

    class Lock {
    public:
      explicit Lock(TidexMutex& m, ThreadID threadID) : m(m), nextEgress(m.lock(threadID)) {}
      explicit Lock(TidexMutex& m): m(m), nextEgress(m.lock(cpu::hwThreadID())){}
      Lock(Lock const&) = delete;
      void operator=(Lock const&) = delete;
      ~Lock() { m.unlock(nextEgress); }
    private:
      TidexMutex& m;
      ThreadID nextEgress;
    };
    
    ThreadID lock(ThreadID mytid) {
      // if this thread was the last unlocking thread negate the thread id 
      // to make this cycle distinguishable from the last for other threads
      // Used to prevent a break of the mutex
      if (egress == mytid) mytid = ThreadID(~mytid);

      // replace the ingress thread id by our own and save the old one
      ThreadID prevtid = ingress.exchange(mytid);

      // while there are threads waiting in front of this thread delay execution
      while (egress.load(std::memory_order_acquire) != prevtid) { 
	hwthread_pause(); 
      }
		
      // store our thread id (maybe negated) for unlocking
      return mytid;
    }

    void unlock(ThreadID nextEgress) { egress.store(nextEgress, std::memory_order_release); }
  };
 
} // namespace mythos
