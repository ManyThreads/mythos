#pragma once

#include <cstdint>
#include "util/LinkedList.hh"
#include "signal.hh"

namespace mythos {

class IKeventSource;
class IKeventSink;

typedef uint64_t UserContext;

struct KEvent
{
    UserContext context;
    Signal value;
};

class IKEventSource
{
public:
    virtual ~IKEventSource() {}

    virtual KEvent deliverKEvent() = 0;

    virtual void attachedToKEventSink(IKEventSink*) {}

    virtual void detachedFromKEventSink(IKEventSink*) {}
};

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
