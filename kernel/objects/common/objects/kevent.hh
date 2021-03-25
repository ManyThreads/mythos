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

class IKeventSource;
class IKeventSink;

/** notifies a single observer about a pending event.
 * 
 * A sink can keep track of multiple sources with a pending event.
 * Each source can enqueue to just a single sink. 
 * The sink pulls the event from the source when it is ready to process it.
 * Thus, the event has to convey context information about the source to the sink. 
 * This is achieved by a user-configurable context value.
 * The combination of user context and event data is designed such that it can be forwarded to the user-space. 
 * The KEvent contains the 64bit user-context and the 64bit signal value.
 * The source implementation provides the list handle for the sink's list.
 */
class IKEventSource
{
public:
    virtual ~IKEventSource() {}

    virtual KEvent deliverKEvent() = 0;

    virtual void attachedToKEventSink(IKEventSink*) {}

    virtual void detachedFromKEventSink(IKEventSink*) {}
};

/** queue of pending events, which are pulled later for delivery to user space.
 * 
 * A sink can keep track of multiple sources with a pending event.
 * Each source can enqueue to just a single sink. 
 * The sink pulls the event from the source when it is ready to process it.
 * Thus, the event has to convey context information about the source to the sink. 
 * This is achieved by a user-configurable context value.
 * The combination of user context and event data is designed such that it can be forwarded to the user-space. 
 * The KEvent contains the 64bit user-context and the 64bit signal value.
 * The source implementation provides the list handle for the sink's list.
 */
class IKEventSink
{
public:
    typedef LinkedList<IKEventSource*> list_t;
    typedef typename list_t::Queueable handle_t;

    virtual ~IKEventSink() {}

    virtual void attachKEventSink(handle_t*) = 0;

    virtual void detachKEventSink(handle_t*) = 0;

};

} // namespace mythos
