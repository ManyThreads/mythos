#ifndef US_BROADCAST
#define US_BROADCAST

#include "util/SingleLinkedList.hh"
#include "util/Structs.hh"

extern SingleLinkedList jobPool;

//#include "util/US_Broadcast.hh"

void* run(Chain& c);
void init_broadcast(Chain& c);
void* broadcast(void* args);

void* run(Chain& c){
  MLOG_INFO(mlog::app, "Start Broadcast");
  init_broadcast(c);

  return nullptr;
}

void init_broadcast(Chain& c){
  std::atomic<int> res(0);  // response counter
  Chain* tasks = jobPool.get_tasks();  // get an amount of tasks
  int num_tasks = 0;  // counter to determine how many tasks were assigned

  HandlerArg_t h_arg(&c, &res);

  // Set acknowledgement-counter as argument
  // for acknowledgement handling function to access it
  c.setHandlerArgs(&h_arg);

  while(tasks){
    MLOG_INFO(mlog::app, "Main propagate to Thread with Chain:", DVARhex(tasks));
    tasks->setTask(c);
    tasks->unlock();
    tasks = tasks->getNext();
    num_tasks++;

    // If all jobs are propagated try to get new jobs if there are some remaining
    if(!tasks){
      MLOG_INFO(mlog::app, "Main trying to get new Tasks");
      tasks = jobPool.get_tasks();
    }
  }

  MLOG_INFO(mlog::app, "Main waiting for ACK's", DVAR(num_tasks), DVARhex(&res));

  // Poll for acknowledgements
  // Weiterer Evaluationsfall: An dieser Stelle schlafen legen
  while(res < num_tasks);

  MLOG_INFO(mlog::app, "Main Received all ACK's, returning...");
}

void* broadcast(void* args){
  MLOG_INFO(mlog::app, "Some Thread trying to propagate", DVARhex(pthread_self()));
  HandlerArg_t* h_args = reinterpret_cast<HandlerArg_t*>(args);
  Chain* c_old = h_args->task;
  MLOG_INFO(mlog::app, "c_old:", DVARhex(c_old));
  std::atomic<int>* parents_ack_count = h_args->ack_count;
  std::atomic<int> res(0);  // response counter
  Chain* tasks = jobPool.get_tasks();  // get an amount of tasks
  int num_tasks = 0;  // counter to determine how many tasks were assigned
  MLOG_INFO(mlog::app, "TEST");
  Chain c(*c_old);
  HandlerArg_t new_handler_args(&c, &res);
  MLOG_INFO(mlog::app, "Threads propagation initialization complete", DVARhex(pthread_self()));

  // Set acknowledgement-counter as argument
  // for acknowledgement handling function to access it
  c.setHandlerArgs(&new_handler_args);

  while(tasks){
    MLOG_INFO(mlog::app, "Thread propagate to Thread with Chain:", DVARhex(tasks), DVARhex(pthread_self()));
    tasks->setTask(c);
    tasks->unlock();
    tasks = tasks->getNext();
    num_tasks++;

    // If all jobs are propagated try to get new jobs if there are some remaining
    if(!tasks){
      MLOG_INFO(mlog::app, "Thread trying to get new Tasks", DVARhex(pthread_self()));
      tasks = jobPool.get_tasks();
    }
  }

  MLOG_INFO(mlog::app, "Thread waiting for ACK's. Amount to wait for:", DVAR(num_tasks), DVARhex(pthread_self()));

  // Poll for acknowledgements
  // Weiterer Evaluationsfall: An dieser Stelle schlafen legen
  while(res < num_tasks);

  MLOG_INFO(mlog::app, "All acks received going to ack by myself", DVARhex(pthread_self()), DVARhex(parents_ack_count));

  // Acknowledge successful propagation
  *parents_ack_count++;

  MLOG_INFO(mlog::app, "Thread will now return from propagation", DVARhex(pthread_self()));

  return nullptr;
}

#endif // US_BROADCAST
