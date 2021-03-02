#ifndef SINGLELINKEDLISTNONBLOCK_HH
#define SINGLELINKEDLISTNONBLOCK_HH

#include <atomic>

#include "util/Tasklet.hh"

extern const size_t DIVISOR;

class SingleLinkedListNonBlock{
public:
  SingleLinkedListNonBlock();

  void push_front(Tasklet*);
  Tasklet* pop_front();

  Tasklet* getJobs();

  bool empty();
  size_t size();

private:
  std::atomic<Tasklet*> head;
};

SingleLinkedListNonBlock::SingleLinkedListNonBlock(){
  this->head = nullptr;
}

void SingleLinkedListNonBlock::push_front(Tasklet* t){
  Tasklet* old_head;
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

Tasklet* SingleLinkedListNonBlock::pop_front(){
  Tasklet* ret;
  Tasklet* new_head;
  bool done = false;

  do{
    ret = this->head;
    new_head = ret->getNext();
    done = this->head.compare_exchange_weak(ret, new_head);
  } while(!done);


  return ret;
}

Tasklet* SingleLinkedListNonBlock::getJobs(){
  Tasklet* jobs = this->head;
  Tasklet* end = jobs;
  size_t len; // Amount of Tasklets to pull
  bool done = false;

  if(jobs){
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

      Tasklet* tmp = nullptr;
      if(end){
        tmp = end->getNext();
        end->setNext(nullptr);
      }

      done = this->head.compare_exchange_weak(jobs, tmp);
    } while(!done); // Jobs leer?
  }

  return jobs;
}


bool SingleLinkedListNonBlock::empty(){
  return this->head == nullptr;
}

size_t SingleLinkedListNonBlock::size(){
  return this->head.load()->getLength() + 1;
}

#endif // SINGLELINKEDLISTNONBLOCK_HH
