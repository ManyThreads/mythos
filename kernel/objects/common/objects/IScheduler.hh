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
#pragma once

#include "util/LinkedList.hh"
#include "async/IResult.hh"

namespace mythos {

  class IThread;
  class ISchedulable;
  class Tasklet;

  class IScheduler
  {
  public:
    virtual ~IScheduler() {}

    typedef LinkedList<ISchedulable*> list_t;
    typedef typename list_t::Queueable handle_t;

    virtual void bind(handle_t* ec_handle) = 0;

    /** Removes the EC from all queues and pointers. Does not preempt the execution context
     * if it is currently running. This is up to the execution context.
     */
    virtual void unbind(handle_t* ec_handle) = 0;

    /** Notifies the scheduler when an EC became ready to run.
     * If the scheduler's hardware thread is sleeping, it is woken up by this
     * method.
     */
    virtual void ready(handle_t* ec_handle) = 0;

    /** Interrupts the scheduler's hardware thread if it is currently
     * running the referenced Execution Context. Notifies the result sink when done.
     * @param t the Tasklet to use
     * @param res the result sink for the completion notification
     * @param ec_handle the queue handle of the EC that shall be interrupted
     */
    virtual void preempt(Tasklet* t, IResult<void>* res, handle_t* ec_handle) = 0;
  };

} // namespace mythos
