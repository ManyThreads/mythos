#ifndef TASKLET_HH
#define TASKLET_HH

#include "util/Typedef.hh"
#include <cstddef>
#include <pthread.h>
#include <atomic>

class Tasklet{
public:
  Tasklet();
  Tasklet(Funptr_t taskptr, Funptr_t handlerptr, void* task_args, void* handler_args);
  Tasklet(const Tasklet&);

  Tasklet* getNext();
  void setNext(Tasklet*);
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

  void copyTask(Tasklet& task);

  void cond_wait();
  void spin_wait();

  void cond_signal();
  void spin_unlock();

private:
  Tasklet* next;
  size_t length;        // Amount of tasklets that are linked after this one
  Funptr_t taskptr;
  Funptr_t handlerptr;
  void* task_args;
  void* handler_args;
  pthread_mutex_t mutex;
  pthread_cond_t cond;
  std::atomic<bool> barrier;
  size_t pid;
};

Tasklet::Tasklet(){
  this->next = nullptr;
  this->length = 0;
  this->taskptr = nullptr;
  this->handlerptr = nullptr;
  this->task_args = nullptr;
  this->handler_args = nullptr;
  this->mutex = PTHREAD_MUTEX_INITIALIZER;
  this->cond = PTHREAD_COND_INITIALIZER;
  this->barrier = false;
}

Tasklet::Tasklet(Funptr_t taskptr, Funptr_t handlerptr, void* task_args, void* handler_args){
  this->next = nullptr;
  this->length = 0;
  this->taskptr = taskptr;
  this->handlerptr = handlerptr;
  this->task_args = task_args;
  this->handler_args = handler_args;
  this->mutex = PTHREAD_MUTEX_INITIALIZER;
  this->cond = PTHREAD_COND_INITIALIZER;
  this->barrier = false;
}

Tasklet::Tasklet(const Tasklet& t){
  this->next = nullptr;
  this->taskptr = t.taskptr;
  this->handlerptr = t.handlerptr;
  this->task_args = t.task_args;
  this->handler_args = t.handler_args;
}

Tasklet* Tasklet::getNext(){
  return this->next;
}

void Tasklet::setNext(Tasklet* next){
  this->next = next;
}

size_t Tasklet::getLength(){
  return this->length;
}

void Tasklet::setLength(size_t length){
  this->length = length;
}

Funptr_t Tasklet::getTask(){
  return this->taskptr;
}

void Tasklet::setTask(Funptr_t task){
  this->taskptr = task;
}

Funptr_t Tasklet::getHandler(){
  return this->handlerptr;
}

void Tasklet::setHandler(Funptr_t handler){
  this->handlerptr = handler;
}

void* Tasklet::getTaskArgs(){
  return this->task_args;
}

void Tasklet::setTaskArgs(void* task_args){
  this->task_args = task_args;
}

void* Tasklet::getHandlerArgs(){
  return this->handler_args;
}

void Tasklet::setHandlerArgs(void* handler_args){
  this->handler_args = handler_args;
}

size_t Tasklet::getPid(){
  return this->pid;
}

void Tasklet::setPid(size_t pid){
  this->pid = pid;
}


void Tasklet::copyTask(Tasklet& task){
  this->taskptr = task.taskptr;
  this->handlerptr = task.handlerptr;
  this->task_args = task.task_args;
  this->handler_args = task.handler_args;
}

void Tasklet::cond_wait(){
  pthread_mutex_lock(&this->mutex);
  pthread_cond_wait(&this->cond, &this->mutex);
}

void Tasklet::spin_wait(){
  while(!this->barrier);
}

void Tasklet::cond_signal(){
  pthread_mutex_lock(&this->mutex);
  pthread_cond_signal(&this->cond);
  pthread_mutex_unlock(&this->mutex);
}

void Tasklet::spin_unlock(){
  this->barrier = true;
}

#endif // TASKLET_HH
