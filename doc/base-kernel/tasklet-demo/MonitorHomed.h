/* -*- mode:C++; -*- */
#pragma once

#include "Place.h"

/** very simple monitor for asynchronous objects. */
class MonitorHomed
{
public:
  MonitorHomed(Place* home)
    : home(home), openRequests(0), deleteTask(nullptr)
  {}

  void enter() { openRequests++; }

  void release() {
    openRequests--;
    if (openRequests == 0 && deleteTask != nullptr) home->pushShared(deleteTask);
  }

  template<class FUNCTOR>
  void doDelete(Tasklet* msg, FUNCTOR fun) {
    deleteTask = msg->set(fun);
    if (openRequests == 0) home->pushShared(deleteTask);
  }

  template<class FUNCTOR>
  void exclusive(Tasklet* msg, FUNCTOR fun) {
    enter();
    // check if we are on the right thread, then just execute the function
    if (home->isLocal()) {
      trace::nestBegin();
      fun(msg);
      trace::nestEnd();
    }
    else home->pushShared(msg->set(fun));
  }

  template<class FUNCTOR>
  void exclusiveAsync(Tasklet* msg, FUNCTOR fun) {
    enter();
    if (home->isLocal()) home->pushPrivate(msg->set(fun));
    else home->pushShared(msg->set(fun));
  }
 
protected:
  Place* home;
  std::atomic<size_t> openRequests;
  std::atomic<Tasklet*> deleteTask;
};


class ReqMonitorHomed
{
public:
  ReqMonitorHomed(Place* home)
    : home(home), pendingRequests(0), refcount(0), deleteTask(nullptr)
  {}

  template<class FUNCTOR>
  void request(Tasklet* msg, FUNCTOR fun) {
    size_t oldReq = pendingRequests.fetch_add(1);
    if (oldReq == 0) {
      // this is the only thread that was first, now pendingRequests > 0
      // forward the first request to the home thread
      home->pushShared(msg->set(fun));
    } else {
      // other requests were pending, thus store own request in monitor's queue.
      sharedQueue.push(msg->set(fun));
    }
  }

  void requestDone() {
    // This is executed on the home thread.
    // If a pending request is enqueued, we forward the oldest to home's private queue.
    // Another thread could enqueue a request concurrently.
    size_t oldReq = pendingRequests.fetch_add(-1);
    if (oldReq > 1) {
      // we are responsible to schedule the next tasklet
      // will have to wait if it is not yet in the shared queue
      while (true) {
	Tasklet* msg = privateQueue.pop();
	if (msg != nullptr) {
	  home->pushPrivate(msg);
	  return;
	} else {
	  sharedQueue.retrieve(privateQueue);
	  // delay...
	}
      }
    } else {
      // this was the last pending request, check for deletion
      if (refcount == 0 && deleteTask != nullptr) home->pushShared(deleteTask);
    }
  }

  template<class FUNCTOR>
  void response(Tasklet* msg, FUNCTOR fun) {
    acquireRef();
    // check if we are on the right thread, then just execute the function
    if (home->isLocal()) {
      trace::nestBegin();
      fun(msg);
      trace::nestEnd();
    }
    else home->pushShared(msg->set(fun));
  }

  template<class FUNCTOR>
  void responseAsync(Tasklet* msg, FUNCTOR fun) {
    acquireRef();
    if (home->isLocal()) home->pushPrivate(msg->set(fun));
    else home->pushShared(msg->set(fun));
  }
  
  void responseDone() { releaseRef(); }
  
  void acquireRef() { refcount++; }
  void releaseRef() {
    refcount--;
    if (refcount == 0 && pendingRequests==0 && deleteTask != nullptr)
      home->pushShared(deleteTask);
  }

  template<class FUNCTOR>
  void doDelete(Tasklet* msg, FUNCTOR fun) {
    deleteTask = msg->set(fun);
    if (refcount == 0 && pendingRequests==0) home->pushShared(deleteTask);
  }

 
protected:
  Place* home;

  std::atomic<size_t> pendingRequests;
  PrivateTaskletQueue privateQueue; //< accessed only by the owner thread
  SharedTaskletQueue sharedQueue; //< for incoming tasks from other threads
  
  std::atomic<size_t> refcount;
  std::atomic<Tasklet*> deleteTask;
};
