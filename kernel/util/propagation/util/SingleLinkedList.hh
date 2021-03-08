#ifndef SINGLELINKEDLIST_HH
#define SINGLELINKEDLIST_HH

#include <atomic>
#include "util/Chain.hh"

class SingleLinkedList{
public:
  SingleLinkedList();

  void push_front(Chain*);
  Chain* pop_front();

  Chain* getJobs();

  bool empty();
  size_t size();

private:
  std::atomic<Chain*> head;
};

#endif // SINGLELINKEDLIST_HH
