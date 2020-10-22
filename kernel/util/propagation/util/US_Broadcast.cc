#include "util/US_Broadcast.hh"

template <typename T>
US_Broadcast<T>::US_Broadcast(){
  this->jobs = nullptr;
}

template <typename T>
US_Broadcast<T>::US_Broadcast(SingleLinkedList<T>* jobs){
  this->jobs = jobs;
}

template <typename T>
void US_Broadcast<T>::init(SingleLinkedList<T>* jobs){
  thos->jobs = jobs;
}

template <typename T>
void run(Chain<T>* c){
  MLOG_INFO(mlog::app, "Start Broadcast");
  this->init_broadcast(c);
}

template <typename T>
void init_broadcast(Chain<T>* c){
  std::atomic<int> res;
  Chain<T>* tasks = jobs->get_tasks();
  int num_tasks = 0;

  while(tasks){
    // TODO Parameter ändern
    tasks->setValue(c.getValue());

    tasks = tasks->getNext();
    num_tasks++;
  }

  // Polling für erfolgte bestätigungen
  while(num_tasks < res);
}
