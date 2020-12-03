#ifndef STRUCTS_HH
#define STRUCTS_HH

#include "util/Chain.hh"

typedef struct Funparam_t{
  int idx;
  void* (*funptr)(void*);
  void* args;

  Funparam_t(int idx, void* (*funptr)(void*), void* args) : idx(idx), funptr(funptr), args(args){}
  Funparam_t(){}
} Funparam_t;

typedef struct ThreadArg_t{
  Chain* local_mem;
  void* args;
  int id;
  // TODO Verzögerungsvariable

  ThreadArg_t(Chain* local_mem, void* args, int id) : local_mem(local_mem), args(args), id(id){}
} ThreadArg_t;

typedef struct HandlerArg_t{
  Chain* task;
  std::atomic<int>* ack_count;

  HandlerArg_t(){}
  HandlerArg_t(Chain* task, std::atomic<int>* ack_count) : task(task), ack_count(ack_count){}
} HandlerArg_t;

#endif // STRUCTS_HH
