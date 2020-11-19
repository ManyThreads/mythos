#include "util/US_Broadcast.hh"

void US_Broadcast::run(Chain& c){
  MLOG_INFO(mlog::app, "Start Broadcast");
  this->init_broadcast(c);
}

void US_Broadcast::init_broadcast(Chain& c){
  std::atomic<int> res(0);  // response counter
  Chain* tasks = jobPool->get_tasks();  // get an amount of tasks
  int num_tasks = 0;  // counter to determine how many tasks were assigned

  // Set acknowledgement-counter as argument
  // for acknowledgement handling function to access it
  c.setHandlerArgs(&res);

  while(tasks){
    tasks->setTask(c);
    tasks->unlock();
    tasks = task->getNext();
    num_tasks++;

    // If all jobs are propagated try to get new jobs if there are some remaining
    if(!tasks){
      MLOG_INFO(mlog::app, "Main trying to get new Tasks");
      tasks = jobPool->get_tasks();
    }
  }

  MLOG_INFO(mlog::app, "Main waiting for ACK's");

  // Poll for acknowledgements
  // Weiterer Evaluationsfall: An dieser Stelle schlafen legen
  while(res < num_tasks);
}

void broadcast(){
  std::atomic<int> res(0);  // response counter
  Chain* tasks = jobList->get_tasks();  // get an amount of tasks
  int num_tasks = 0;  // counter to determine how many tasks were assigned

  // Set acknowledgement-counter as argument
  // for acknowledgement handling function to access it
  c.setHandlerArgs(&res);

  while(tasks){
    tasks->setTask(c);
    tasks->unlock();
    tasks = task->getNext();
    num_tasks++;

    // If all jobs are propagated try to get new jobs if there are some remaining
    if(!tasks){
      MLOG_INFO(mlog::app, "Thread trying to get new Tasks");
      tasks = this->jobList->get_tasks();
    }
  }

  MLOG_INFO(mlog::app, "Thread waiting for ACK's");

  // Poll for acknowledgements
  // Weiterer Evaluationsfall: An dieser Stelle schlafen legen
  while(res < num_tasks);

  // Acknowledge successful propagation
  res++;
}
