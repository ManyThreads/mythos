#include "util/Chain.hh"

Chain::Chain(){
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

Chain::Chain(Funptr_t taskptr, Funptr_t handlerptr, void* task_args, void* handler_args){
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

Chain::Chain(const Chain& t){
  this->next = nullptr;
  this->taskptr = t.taskptr;
  this->handlerptr = t.handlerptr;
  this->task_args = t.task_args;
  this->handler_args = t.handler_args;
}

Chain* Chain::getNext(){
  return this->next;
}

void Chain::setNext(Chain* next){
  this->next = next;
}

size_t Chain::getLength(){
  return this->length;
}

void Chain::setLength(size_t length){
  this->length = length;
}

Funptr_t Chain::getTask(){
  return this->taskptr;
}

void Chain::setTask(Funptr_t task){
  this->taskptr = task;
}

Funptr_t Chain::getHandler(){
  return this->handlerptr;
}

void Chain::setHandler(Funptr_t handler){
  this->handlerptr = handler;
}

void* Chain::getTaskArgs(){
  return this->task_args;
}

void Chain::setTaskArgs(void* task_args){
  this->task_args = task_args;
}

void* Chain::getHandlerArgs(){
  return this->handler_args;
}

void Chain::setHandlerArgs(void* handler_args){
  this->handler_args = handler_args;
}

size_t Chain::getPid(){
  return this->pid;
}

void Chain::setPid(size_t pid){
  this->pid = pid;
}


void Chain::copyTask(Chain& task){
  this->taskptr = task.taskptr;
  this->handlerptr = task.handlerptr;
  this->task_args = task.task_args;
  this->handler_args = task.handler_args;
}

void Chain::cond_wait(){
  pthread_mutex_lock(&this->mutex);
  while(!barrier){
    pthread_cond_wait(&this->cond, &this->mutex);
  }
  this->barrier = false;
  pthread_mutex_unlock(&this->mutex);
}

void Chain::spin_wait(){
  while(!this->barrier);
  this->barrier = false;
}

void Chain::cond_signal(){
  pthread_mutex_lock(&this->mutex);
  this->barrier = true;
  pthread_cond_signal(&this->cond);
  pthread_mutex_unlock(&this->mutex);
}

void Chain::spin_unlock(){
  this->barrier = true;
}
