#pragma once

#include "util/assert.hh"
#include "mythos/syscall.hh"
#include "mythos/caps.hh"
#include "runtime/ISysretHandler.hh"
#include <pthread.h>

// functions exposing some more mythos threading functionality
// in a safer way then just using the bare system calls
// naming is designed to go well with pthread library calls


inline void mythos_wait()
{
  mythos::ISysretHandler::handle(mythos::syscall_wait());
}

inline mythos::CapPtr mythos_get_pthread_ec(pthread_t pthread)
{
  return mythos::CapPtr(mythos_get_pthread_tid(pthread));
}

inline mythos::CapPtr mythos_get_pthread_ec_self()
{
  return mythos_get_pthread_ec(pthread_self());
}

inline void mythos_signal_pthread(pthread_t pthread)
{
  auto result = mythos::syscall_signal(mythos_get_pthread_ec(pthread));
  ASSERT(mythos::Error(result.state) == mythos::Error::SUCCESS);
}
