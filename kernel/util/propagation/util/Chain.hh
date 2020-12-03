#ifndef CHAIN_HH
#define CHAIN_HH

#include "util/Typedef.hh"

//#include <atomic>

class Chain{
public:
  Chain();
  Chain(const Chain& c);
  ~Chain() = default;

  void init(Funptr_t task, void* fun_args, Funptr_t handler, void* handler_args);

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
  void setTask(Chain* c);
  void unlock();
  void lock();
  int getBarrierState();

private:
  Chain* next;
  Funptr_t funptr;
  Funptr_t handlerptr;
  void* fun_args;
  void* handler_args;
  std::atomic<int> barrier;
};

Chain::Chain(){
  this->next = nullptr;
  this->barrier.store(0);
}

Chain::Chain(const Chain& c){
  MLOG_INFO(mlog::app,"MIAU");
  this->next = nullptr;
  MLOG_INFO(mlog::app,"MIAU1");
  this->funptr = c.funptr;
  MLOG_INFO(mlog::app,"MIAU2");
  this->handlerptr = c.handlerptr;
  MLOG_INFO(mlog::app,"MIAU3");
  this->fun_args = c.fun_args;
  MLOG_INFO(mlog::app,"MIAU4");
  this->barrier.store(0);
  MLOG_INFO(mlog::app,"MIAU5");
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
