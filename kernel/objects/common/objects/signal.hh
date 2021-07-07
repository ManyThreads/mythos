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
 * Copyright 2021 Randolf Rotta, Robert Kuban, and contributors, BTU Cottbus-Senftenberg
 */
#pragma once

#include <cstdint>
#include "util/LinkedList.hh"
#include "mythos/KEvent.hh"

namespace mythos {

class ISignalSource;
class ISignalSink;

/** Signals are 64bit values to match the KEvent values.
 * A 32bit value would be good enough probably.
 */ 
typedef KEvent::Value Signal;

/** pushes signal values to multiple observers.
 * 
 * A Signal is pushed from a ISignalSource to a ISignalSink. 
 * A source can keep track of multiple sinks. 
 * Each sink observes just a single source. 
 * Thus, the Signal does not need to convey context information about the source. 
 * The sink implementation provides the list handle for the source's list.
 */
class ISignalSource
{
public:
    typedef LinkedList<ISignalSink*> list_t;
    typedef typename list_t::Queueable handle_t;

    virtual ~ISignalSource() {}

    virtual void attachSignalSink(handle_t*) = 0;

    virtual void detachSignalSink(handle_t*) = 0;
};

/** receives pushed signal values from a single observable.
 * 
 * A Signal is pushed from a ISignalSource to a ISignalSink. 
 * A source can keep track of multiple sinks. 
 * Each sink observes just a single source. 
 * Thus, the Signal does not need to convey context information about the source. 
 * The sink implementation provides the list handle for the source's list.
 */
class ISignalSink
{
public:
    virtual ~ISignalSink() {}

    virtual void signalChanged(Signal newValue) = 0;
};

} // namespace mythos
