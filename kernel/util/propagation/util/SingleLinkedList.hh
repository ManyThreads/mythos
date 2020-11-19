#ifndef SINGLELINKEDLIST_HH
#define SINGLELINKEDLIST_HH

#include <atomic>
//#include "PredefinedData.hh"
#include "util/Chain.hh"

extern const size_t MAX_TASK;
extern const size_t TASKQUEUE_DIVISOR;

class SingleLinkedList{
public:
  enum STATE : std::uint8_t{
    READY,
    BLOCKED
  };

  SingleLinkedList();
  SingleLinkedList(Chain* init);
  ~SingleLinkedList() = default;

  void push_front(Chain* chain);
  Chain* pop_front();

  size_t getSize();

  Chain* get_tasks();

  void print();

private:
  Chain* head;
  size_t size;
  std::atomic<STATE> state;
};

SingleLinkedList::SingleLinkedList(){
  this->head = nullptr;
  this->size = 0;
  this->state = READY;
}

SingleLinkedList::SingleLinkedList(Chain* init){
  this->head = nullptr;
  this->size = 0;
  this->state = READY;

  for(size_t i = 0; i < MAX_TASK; i++){
    this->push_front(&init[i]);
  }
}

void SingleLinkedList::push_front(Chain* chain){
  STATE s = READY;

  while(!this->state.compare_exchange_weak(s, BLOCKED)){
    s = READY;
  }

  chain->setNext(this->head);
  this->head = chain;
  this->size += 1;

  this->state.store(READY);
}

Chain* SingleLinkedList::pop_front(){
  Chain* ret = nullptr;
  STATE s = READY;

  while(!this->state.compare_exchange_weak(s, BLOCKED)){
    s = READY;
  }

  ret = this->head;

  if(!ret){
    goto exit;
  }

  this->head = this->head->getNext();
  this->size -= 1;

  // For Securety reasons set next of returned value to 0
  ret->setNext(nullptr);

exit:
  this->state.store(READY);

  return ret;
}

size_t SingleLinkedList::getSize(){
  return this->size;
}

Chain* SingleLinkedList::get_tasks(){
  STATE s = READY;

  int amount_tasks;
  int count;

  Chain* start = nullptr;
  Chain* end;

  while(!this->state.compare_exchange_weak(s, BLOCKED)){
    s = READY;
  }

  if(!this->size){
    goto exit;
  }

  amount_tasks = this->size / TASKQUEUE_DIVISOR;
  count = amount_tasks - 1;

  start = this->head;
  end = start;

  while(count > 0){
    end = end->getNext();
    count--;
  }

  if(end){
    this->head = end->getNext();
    this->size = this->size - amount_tasks;
    end->setNext(nullptr);
  } else{
    this->size = 0;
    this->head = nullptr;
  }


exit:
  this->state.store(READY);

  return start;
}

void SingleLinkedList::print(){
  Chain* chain = this->head;
  size_t i = 0;

  //std::cout << "Size: " << this->size << std::endl;
  MLOG_INFO(mlog::app, "Size: ", DVAR(this->size));

  while(chain){
    //std::cout << "Index: " << i << ", " << chain << std::endl;
    MLOG_INFO(mlog::app, "Index/ptr: ", DVAR(i), DVARhex(chain));
    chain = chain->getNext();
    i++;
  }
}

#endif // SINGLELINKEDLIST_HH
