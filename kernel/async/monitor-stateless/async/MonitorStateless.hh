/* -*- mode:C++; -*- */
#pragma once

#include "util/assert.hh"
#include "async/Place.hh"

namespace mythos {
namespace async {

/**
 *  Simple monitor for stateless asyncronous objects.
 */

class MonitorStateless
{
public:

  template<class FUNCTOR>
  void async(Tasklet* msg, FUNCTOR fun) {
    getLocalPlace().pushShared(msg->set(fun));
  }

};

} // async
} //mythos
