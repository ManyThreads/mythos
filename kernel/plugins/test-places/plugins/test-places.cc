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

#include "plugins/Plugin.hh"
#include "async/Place.hh"
#include "async/NestedMonitorHome.hh"
#include "async/NestedMonitorDelegating.hh"
#include "async/SimpleMonitorHome.hh"
#include "async/NestedMonitorDummy.hh"
#include "boot/mlog.hh"
#include "cpu/hwthreadid.hh"

namespace mythos {

  class TestPlaces
    : public Plugin
  {
  public:
    struct Node {
      Node() {}

      void run() {
	monitor.request(&starter, [=](Tasklet*) {
	    MLOG_INFO(mlog::boot, "TestPlaces: started processing");
	    state = 0;
	    process();
	  });
      }

      void process() {
	switch (state) {
	case 0:
	  for (i=0; i<cpu::getNumThreads(); i++) {
	    if(&nodes[i] == this) continue;
	    state = 1;
	    nodes[i].fooRequest(&mylet, this);
	    return this->monitor.responseDone();
	  case 1: ;
	  }
	  MLOG_INFO(mlog::boot, "TestPlaces: finished roundtrip");
	  state = 2;
	  return this->monitor.responseAndRequestDone();
	default:
	  MLOG_INFO(mlog::boot, "TestPlaces: strange call");
	}
      }

      void fooRequest(Tasklet* t, Node* r) {
	MLOG_INFO(mlog::boot, "TestPlaces: sending foo request", DVAR(t), DVAR(r), "to", DVAR(this));
	monitor.request(t, [=](Tasklet*t) { this->_fooRequest(t,r); } );
      }

      void _fooRequest(Tasklet* t, Node* r) {
	MLOG_INFO(mlog::boot, "TestPlaces: process foo request", DVAR(t), DVAR(r), "on", DVAR(this));
	r->fooResponse(t);
	monitor.requestDone();
      }

      void fooResponse(Tasklet*t) {
	MLOG_INFO(mlog::boot, "TestPlaces: sending foo response", DVAR(t), "to", DVAR(this));
	monitor.response(t, [=](Tasklet*) { this->process(); });
      }

      async::NestedMonitorHome monitor;
      int state;
      size_t i;
      Tasklet mylet;
      Tasklet starter;
    };

    static Node nodes[MYTHOS_MAX_THREADS];

    void initGlobal() {
      for (size_t id=0; id<cpu::getNumThreads(); id++) {
	nodes[id].monitor.setHome(&async::places[id]);
      }
    }

    /// @todo should be thread instead of apic ID!
    void initThread(cpu::ThreadID threadID) {
      MLOG_DETAIL(mlog::boot, "TestPlaces: my place is", &mythos::async::getLocalPlace(),
                  DVAR(threadID), cpu::getNumThreads());
      if (threadID == 0) {
	nodes[0].run();
      }
    }
  };

  TestPlaces::Node TestPlaces::nodes[MYTHOS_MAX_THREADS];
  TestPlaces plugin_TestPlaces;

} // namespace mythos
