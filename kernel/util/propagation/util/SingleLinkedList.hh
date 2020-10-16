#ifndef SINGLELINKEDLIST_HH
#define SINGLELINKEDLIST_HH

#include <atomic>
//#include "PredefinedData.hh"
#include "util/Chain.hh"

extern const size_t MAX_TASK;
extern const size_t TASKQUEUE_DIVISOR;

template <typename T>
class SingleLinkedList{
public:
  enum STATE : std::uint8_t{
    READY,
    BLOCKED
  };

  SingleLinkedList();
  SingleLinkedList(Chain<T>* init);
  ~SingleLinkedList() = default;

  void push_front(Chain<T>* chain);
  Chain<T>* pop_front();

  size_t getSize();

  Chain<T>* get_tasks();

  void print();

private:
  Chain<T>* head;
  size_t size;
  std::atomic<STATE> state;
};

template <typename T>
SingleLinkedList<T>::SingleLinkedList(){
  this->head = nullptr;
  this->size = 0;
  this->state = READY;
}

template <typename T>
SingleLinkedList<T>::SingleLinkedList(Chain<T>* init){
  this->head = nullptr;
  this->size = 0;
  this->state = READY;

  for(size_t i = 0; i < MAX_TASK; i++){
    this->push_front(&init[i]);
  }
}

template <typename T>
void SingleLinkedList<T>::push_front(Chain<T>* chain){
  STATE s = READY;

  while(!this->state.compare_exchange_weak(s, BLOCKED)){
    s = READY;
  }

  chain->setNext(this->head);
  this->head = chain;
  this->size += 1;

  this->state.store(READY);
}

template <typename T>
Chain<T>* SingleLinkedList<T>::pop_front(){
  Chain<T>* ret = nullptr;
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

template <typename T>
size_t SingleLinkedList<T>::getSize(){
  return this->size;
}

template <typename T>
Chain<T>* SingleLinkedList<T>::get_tasks(){
  STATE s = READY;

  int amount_tasks;
  int count;

  Chain<T>* start;
  Chain<T>* end;

  while(!this->state.compare_exchange_weak(s, BLOCKED)){
    s = READY;
  }

  if(!this->size){
    goto exit;
  }

  amount_tasks = this->size / TASKQUEUE_DIVISOR;
  count = amount_tasks;

  start = this->head;
  end = start;

  while(count > 0){
    end = end->getNext();
    count--;
  }

  this->head = end->getNext();

  this->size = this->size - amount_tasks;

exit:
  this->state.store(READY);

  return start;
}

template <typename T>
void SingleLinkedList<T>::print(){
  Chain<T>* chain = this->head;
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
