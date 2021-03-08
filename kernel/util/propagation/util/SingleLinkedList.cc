#include "util/SingleLinkedList.hh"

extern size_t DIVISOR;

SingleLinkedList::SingleLinkedList(){
  this->head = nullptr;
}

void SingleLinkedList::push_front(Chain* t){
  Chain* old_head;
  bool done = false;

  do{
    old_head = this->head;

    if(!old_head){
      t->setLength(0);
    } else {
      t->setLength(old_head->getLength() + 1);
    }

    t->setNext(old_head);
    done = this->head.compare_exchange_weak(old_head, t);
  } while(!done);
}

Chain* SingleLinkedList::pop_front(){
  Chain* ret;
  Chain* new_head;
  bool done = false;

  do{
    ret = this->head;
    new_head = ret->getNext();
    done = this->head.compare_exchange_weak(ret, new_head);
  } while(!done);


  return ret;
}

Chain* SingleLinkedList::getJobs(){
  Chain* jobs = nullptr;// = this->head;
  Chain* end = jobs;
  size_t len; // Amount of Chains to pull
  bool done = false;

  //if(jobs){
    do{
      jobs = this->head;
      if(!jobs){
        break;
      }
      end = jobs;
      len = (jobs->getLength() + 1) / DIVISOR;

      for(size_t i = 0; i < len - 1 && end != nullptr; i++){
        end = end->getNext();
      }

      Chain* tmp = nullptr;
      if(end){
        tmp = end->getNext();
      }

      done = this->head.compare_exchange_weak(jobs, tmp);
    } while(!done); // Jobs leer?
  //}
  if(end){
    end->setNext(nullptr);
  }

  return jobs;
}


bool SingleLinkedList::empty(){
  return this->head == nullptr;
}

size_t SingleLinkedList::size(){
  size_t ret = 0;
  Chain* head = this->head;
  if(head){
    ret =  head->getLength() + 1;
  }

  return ret;
}
