#ifndef CHAIN_HH
#define CHAIN_HH

#include "util/Typedef.hh"

//#include <atomic>

class Chain{
public:
  Chain();

  /**
   * Copies function pointers and their arguments,
   * sets next to nullptr and locks the barrier
   */
  Chain(const Chain& c);
  ~Chain() = default;

  void init(Funptr_t task, void* fun_args, Funptr_t handler, void* handler_args);

  /*
    Getters and setters working as one would expect them to work like
  */
  Funptr_t getFunptr();
  void setFunptr(Funptr_t funptr);
  void* getFunArgs();
  void setFunArgs(void* args);

  Funptr_t getHandler();
  void setHandler(Funptr_t handler);
  void* getHandlerArgs();
  void setHandlerArgs(void* args);

  Chain* getNext();
  void setNext(Chain* next);

  int getBarrierState();

  /**
   * Sets function pointers and arguments of this to those of c
   */
  void setTask(Chain& c);

  /**
   * Sets function pointers and arguments of this to those of c
   */
  void setTask(Chain* c);

  /**
   * Unlocks the barrier variable by setting it to 1
   */
  void unlock();

  /**
   * Locks the barrier variable by setting it to 0
   */
  void lock();

private:
  Chain* next;              // Next Chain element in List
  Funptr_t funptr;          // Pointer to the function that shall be executed before further Propagation
  Funptr_t handlerptr;      // Pointer to propagation function that will be executed after the given task in funptr was executed
  void* fun_args;           // Arguments for funptr
  void* handler_args;       // Arguments for handlerptr
  std::atomic<int> barrier; // Barrier for initial Thread synchronization in Testcases
};

Chain::Chain(){
  this->next = nullptr;
  this->barrier.store(0);
}

Chain::Chain(const Chain& c){
  this->next = nullptr;
  this->funptr = c.funptr;
  this->handlerptr = c.handlerptr;
  this->fun_args = c.fun_args;
  this->barrier.store(0);
}

void Chain::init(Funptr_t task, void* fun_args, Funptr_t handler, void* handler_args){
  this->next = nullptr;
  this->funptr = task;
  this->fun_args = fun_args;
  this->handlerptr = handler;
  this->handler_args = handler_args;
  this->barrier.store(0);
}

Funptr_t Chain::getFunptr(){
  return this->funptr;
}

void Chain::setFunptr(Funptr_t funptr){
  this->funptr = funptr;
}

void* Chain::getFunArgs(){
  return this->fun_args;
}

void Chain::setFunArgs(void* args){
  this->fun_args = args;
}

Funptr_t Chain::getHandler(){
  return this->handlerptr;
}

void Chain::setHandler(Funptr_t handler){
  this->handlerptr = handler;
}

void* Chain::getHandlerArgs(){
  return this->handler_args;
}

void Chain::setHandlerArgs(void* args){
  this->handler_args = args;
}

Chain* Chain::getNext(){
  return this->next;
}

void Chain::setNext(Chain* next){
  this->next = next;
}

void Chain::setTask(Chain& c){
  this->funptr = c.funptr;
  this->handlerptr = c.handlerptr;
  this->fun_args = c.fun_args;
  this->handler_args = c.handler_args;
}

void Chain::setTask(Chain* c){
  this->funptr = c->funptr;
  this->handlerptr = c->handlerptr;
  this->fun_args = c->fun_args;
  this->handler_args = c->handler_args;
}

void Chain::unlock(){
  this->barrier.store(1);
}

void Chain::lock(){
  this->barrier.store(0);
}

int Chain::getBarrierState(){
  return this->barrier;
}

#endif // CHAIN_HH
