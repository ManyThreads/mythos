#ifndef CHAIN_HH
#define CHAIN_HH

#include "util/Typedef.hh"
#include <cstddef>
#include <pthread.h>
#include <atomic>

class Chain{
public:
  Chain();
  Chain(Funptr_t taskptr, Funptr_t handlerptr, void* task_args, void* handler_args);
  Chain(const Chain&);

  Chain* getNext();
  void setNext(Chain*);
  size_t getLength();
  void setLength(size_t);
  Funptr_t getTask();
  void setTask(Funptr_t);
  Funptr_t getHandler();
  void setHandler(Funptr_t);
  void* getTaskArgs();
  void setTaskArgs(void*);
  void* getHandlerArgs();
  void setHandlerArgs(void*);
  size_t getPid();
  void setPid(size_t pid);

  void copyTask(Chain& task);

  void cond_wait();
  void spin_wait();

  void cond_signal();
  void spin_unlock();

private:
  Chain* next;
  size_t length;        // Amount of Chains that are linked after this one
  Funptr_t taskptr;
  Funptr_t handlerptr;
  void* task_args;
  void* handler_args;
  pthread_mutex_t mutex;
  pthread_cond_t cond;
  std::atomic<bool> barrier;
  size_t pid;
};

#endif // CHAIN_HH
