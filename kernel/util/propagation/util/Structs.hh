#ifndef STRUCTS_HH
#define STRUCTS_HH

#include "util/Chain.hh"

/**
 * struct to passthrough function pointer, Threadid and function arguments to
 * thread's main function during initialization function
 */
typedef struct Funparam_t{
  int idx;
  void* (*funptr)(void*);
  void* args;

  Funparam_t(int idx, void* (*funptr)(void*), void* args) : idx(idx), funptr(funptr), args(args){}
  Funparam_t(){}
} Funparam_t;

/**
 * struct to pass needed variables for the thread in it's main function
 * as well as for debugging
 */
typedef struct ThreadArg_t{
  Chain* local_mem;
  void* args;
  int id;
  // TODO Verzögerungsvariable

  ThreadArg_t(Chain* local_mem, void* args, int id) : local_mem(local_mem), args(args), id(id){}
} ThreadArg_t;

/**
 * struct to pass the chain element which contains information about what to do
 * during propagation as well as the acknowledgement counter of parent thread
 * to threads which are later then the parent thread in propagation topology.
 */
typedef struct HandlerArg_t{
  Chain* task;
  std::atomic<int>* ack_count;

  HandlerArg_t(){}
  HandlerArg_t(Chain* task, std::atomic<int>* ack_count) : task(task), ack_count(ack_count){}
} HandlerArg_t;

#endif // STRUCTS_HH
