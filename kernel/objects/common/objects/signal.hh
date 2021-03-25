#pragma once

#include <cstdint>
#include "util/LinkedList.hh"

namespace mythos {

class ISignalSource;
class ISignalSink;

typedef uint64_t Signal;

class ISignalSource
{
public:
    typedef LinkedList<ISignalSink*> list_t;
    typedef typename list_t::Queueable handle_t;

    virtual ~ISignalSource() {}

    virtual void attachSignalSink(handle_t*) = 0;

    virtual void detachSignalSink(handle_t*) = 0;

    virtual updateSignals(Signal setMask, Signal orMask) {};
};

class ISignalSink
{
public:
    virtual ~ISignalSink() {}

    virtual void signalUpdated(Signal newValue) = 0;

    virtual void attachedToSignalSource(ISignalSource*) {};

    virtual void detachedFromSignalSource(ISignalSource*) {};
};

} // namespace mythos
