/* -*- mode:C++; -*- */
#pragma once

#include <atomic>
#include <new>

/** Base class for dummy Tasklet objects, which are sometimes needed for queue management. */
class TaskletBase
{
public:
  std::atomic<TaskletBase*> nextTasklet;
};

/** Actual Tasklet implementation. All users of Tasklets expect at least this size of one cacheline. */
// should be "class alignas(64) Tasklet", but emacs code formatting gets confused :(
class alignas(64) Tasklet 
  : public TaskletBase
{
public:
  enum Consts { CLSIZE = 64 };
  typedef void(*FunPtr)(Tasklet*);
  
  Tasklet() : handler(nullptr) {}

  void run() { handler(this); }

  template<class FUNCTOR>
  Tasklet* set(FUNCTOR fun) {
    static_assert(sizeof(FUNCTOR) <= sizeof(payload), "tasklet payload is too big");
    new(payload) FUNCTOR(fun); // copy-construct
    this->handler = &wrapper<FUNCTOR>;
    return this;
  }

  // safer to return per copy (MSG without const&) compiler will optimize anyway!
  template<class MSG>
  MSG get() {
    static_assert(sizeof(MSG) <= sizeof(payload), "tasklet payload is too big");
    return *reinterpret_cast<MSG*>(payload);
  }

protected:
  template<class FUNCTOR>
  static void wrapper(Tasklet* msg) { msg->get<FUNCTOR>()(msg); }

private:
  FunPtr handler;
  char payload[CLSIZE - sizeof(TaskletBase) - sizeof(handler)];
};
