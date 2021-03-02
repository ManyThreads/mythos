#ifndef TYPEDEF_HH
#define TYPEDEF_HH

#include <cstddef>
#include <atomic>
#include <pthread.h>

typedef void* (*Funptr_t)(void*);
class Tasklet;

typedef struct ThreadInit{
  size_t pid;
  Funptr_t mainFun;
  void* funArgs;
  Tasklet* tasklet;
  ThreadInit(){}
  ThreadInit(size_t i, Funptr_t fun, void* args) : pid(i), mainFun(fun), funArgs(args){}
} ThreadInit;

typedef struct BCArgs{
  std::atomic<size_t>* count;
  Tasklet* task;
  pthread_mutex_t* parentMutex;
  pthread_cond_t* parentCond;

  BCArgs(std::atomic<size_t>* count, Tasklet* task, pthread_mutex_t* mutex, pthread_cond_t* cond) : count(count), task(task), parentMutex(mutex), parentCond(cond) {}
} BCArgs;

#endif // TYPEDEF_HH
