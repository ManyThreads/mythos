/* -*- mode:C++; -*- */
#pragma once

#include "Tasklet.h"
#include "trace.h"
#include <atomic>
#include <semaphore.h>
#include <assert.h>

/** LIFO stack for local task management.
 * nextTasklet points to the previous message.
 * tail points to the top of the stack. 
 */
class PrivateTaskletQueue
{
public:
  PrivateTaskletQueue() : tail(nullptr) {}
  
  bool push(TaskletBase* msg) {
    TaskletBase* oldtail = tail.exchange(msg, std::memory_order_relaxed);
    msg->nextTasklet.store(oldtail, std::memory_order_relaxed);
    return oldtail == nullptr; // true if this was the first message in the queue
  }

  bool isEmpty() const { return tail.load(std::memory_order_relaxed) != nullptr; }

  Tasklet* pop() {
    TaskletBase* oldtail = tail.load(std::memory_order_relaxed);
    if (oldtail != nullptr) {
      tail = oldtail->nextTasklet.load(std::memory_order_relaxed);
      return static_cast<Tasklet*>(oldtail);
    }
    return nullptr;
  }
  
protected:
  std::atomic<TaskletBase*> tail;
};

/** wait-free LIFO stack for incoming tasks.
 * nextTasklet points to the previous message. 
 * tail points to the end of the queue.
 * shifting the tasklets into the private queue, reproduces FIFO order.
 */
class SharedTaskletQueue
{
public: 
  SharedTaskletQueue() : tail(nullptr) {}
  
  bool push(TaskletBase* msg) {
    msg->nextTasklet = (TaskletBase*)1; // mark as incomplete push
    TaskletBase* oldtail = tail.exchange(msg, std::memory_order_relaxed); // replace the tail
    msg->nextTasklet.store(oldtail, std::memory_order_release); // comlete the push
    return oldtail == nullptr; // true if this was the first message in the queue
  }

  void retrieve(PrivateTaskletQueue& q) {
    TaskletBase* oldtail = tail.exchange(nullptr, std::memory_order_relaxed);
    while (oldtail != nullptr) {
      TaskletBase* next;
      do { // wait until the push is no longer marked as incomplete
	// use exchange in order to avoid shared state of cacheline
	next = oldtail->nextTasklet.exchange((TaskletBase*)1, std::memory_order_acquire);
	if (next != (TaskletBase*)1) break;
	// sleep for a short amount of cycles
	asm volatile ("pause");
      } while (true);
      // TODO prefetch next with write hint
      q.push(oldtail);
      oldtail = next;
    }
  }
  
protected:
  std::atomic<TaskletBase*> tail;
};


extern thread_local size_t threadID;

/** Local scheduling for a single thread. 
 * Should be allocated into a local cache line.
 */
class Place
{
public:
  Place(size_t rank)
    : done(false), ownID(rank)
  {
    threadID = rank;
    sem_init(&sleeper, 0, 0);
  }

  void setDone() { done = true; }

  bool isLocal() const { return ownID == threadID; }
  size_t getID() const { return ownID; }
  static size_t id() { return threadID; }
  
  void pushPrivate(Tasklet* msg) {
    assert(isLocal());
    privateQueue.push(msg);
  }

  void pushShared(Tasklet* msg) {
    bool first = sharedQueue.push(msg);
    // home needs to be woken up on first message in the queue
    if (first) {
      sem_post(&sleeper);
    }
  }

  Tasklet* pop() {
    Tasklet* msg = privateQueue.pop();
    if (msg == nullptr) {
      sharedQueue.retrieve(privateQueue);
      msg = privateQueue.pop();
    }
    return msg;
  }

  void loop() {
    while (true) {
      Tasklet* msg = pop();
      if (msg != nullptr) {
	msg->run();
      } else {
	if (done) return;
	// sleep, unless we got new messages
	sem_wait(&sleeper);
      }
    }
  }
  
protected:
  PrivateTaskletQueue privateQueue; //< accessed only by the owner thread
  SharedTaskletQueue sharedQueue; //< for incoming tasks from other threads
  std::atomic<bool> done;
  size_t ownID;
  sem_t sleeper;
};


class ExitService
{
public:
  ExitService(Place** places, size_t numPlaces)
    : places(places), numPlaces(numPlaces), state(0) {}
  
  void exitScheduler() {
    trace::recv(&exitMsg, this, "ExitService::exitScheduler");
    places[0]->pushShared(exitMsg.set( [=](Tasklet* m) { this->exitImpl(); } ));
  }

protected:
  void exitImpl() {
    trace::begin(&exitMsg, this, "ExitService::exitScheduler");
    Place* p;
    switch (state) {
    case 0:
      for (i=1; i<numPlaces; i++) {
	state = 1;
	trace::recv(&exitMsg, this, "ExitService::doExit");
	p = places[i];
	places[i]->pushShared(exitMsg.set([=](Tasklet* m) { this->doExit(p); } ));
	return;
      case 1: ;
      }
      state = 2;
      places[0]->setDone();
    };
  }

  void doExit(Place* p) {
    trace::begin(&exitMsg, this, "ExitService::doExit");
    p->setDone();
    places[0]->pushShared(exitMsg.set( [=](Tasklet* m) { this->exitImpl(); } ));
  }
  
protected:
  Place** places;
  size_t numPlaces;
  size_t state;
  size_t i;
  Tasklet exitMsg;
};
