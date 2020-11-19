#ifndef CHAIN_HH
#define CHAIN_HH

#include "util/Typedef.hh"

#include <atomic>

class Chain{
public:
  Chain();
  ~Chain() = default;

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

  void setTask(Chain& c);
  void unlock();
  void lock();

private:
  Chain* next;
  Funptr_t funptr;
  Handlerptr_t handlerptr;
  void* fun_args;
  void* handler_args;
  std::atomic<int> barrier;
};

Chain::Chain(){
  this->next = nullptr;
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

Handlerptr_t Chain::getHandler(){
  return this->handlerptr;
}

void Chain::setHandler(Handlerptr_t handler){
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

void unlock(){
  this->barrier.store(1);
}

void lock(){
  this->barrier.store(0);
}

#endif // CHAIN_HH
