/* -*- mode:C++; -*- */
#pragma once

#include <vector>
#include <pthread.h>
#include <cstring>
#include <atomic>

class GroupThread;
class ThreadGroup;
class Task;

/** Thread starter for a group of threads. The thread will be pinned
 * to a specific logical processor core. After startup, the group's
 * main method will be called.
 */
class GroupThread
{
public:
  size_t rank() const { return mrank; }
  ThreadGroup* group() const { return mgroup; }

protected:
  friend class ThreadGroup;
  void start(size_t rank, ThreadGroup* group);
  void join() { pthread_join(mthread, NULL); }

  static void* startupHelper(void* t) {
    static_cast<GroupThread*>(t)->startup();
    return nullptr;
  }
  void startup();

protected:
  size_t mrank;
  ThreadGroup* mgroup;
  pthread_t mthread;
};


class ThreadGroup
{
public:
  ThreadGroup(size_t size)
    : vcores(size, 0), members(size), running(false)
  {
    pthread_barrier_init(&groupBarrier, NULL, size);
  }

  ~ThreadGroup() {
    if (running) join();
    pthread_barrier_destroy(&groupBarrier);
  }
  
  size_t size() const { return members.size(); }
  size_t vcore(size_t rank) const { return vcores[rank]; }
  void setVcore(size_t rank, size_t vcore) { vcores[rank] = vcore; }

  void start(Task* task);
  void barrier() { pthread_barrier_wait(&groupBarrier); }
  void join();

protected:
  friend class GroupThread;
  void main(size_t rank);
   
protected:
  std::vector<size_t> vcores;
  std::vector<GroupThread> members;
  pthread_barrier_t groupBarrier;
  Task* task;
  bool running;
};


class Task
{
public:
  Task() : mgroup(nullptr) {}
  virtual ~Task() {}

  virtual void init() {}
  virtual void run(size_t rank) = 0;

  size_t numThreads() const { return mgroup->size(); }
  void barrier() const { mgroup->barrier(); }
  
protected:
  friend class ThreadGroup;
  void setThreadGroup(ThreadGroup* g) { mgroup = g; }
  ThreadGroup* mgroup;
};


class BatchSyncTask
  : public Task
{
public:
  BatchSyncTask() {}
  virtual ~BatchSyncTask() {}

  virtual bool running() = 0;
  virtual void loop(size_t rank) = 0;
  virtual void step() = 0;

private:
  virtual void run(size_t rank) {
    while (running()) {
      loop(rank);
      barrier();
      if (rank == 0) step();
      barrier();
    }
  }
};


inline void ThreadGroup::start(Task* task)
{
  if (running) join(); // wait until previous work is done
  if (size() == 0) return;
  running = true;
  this->task = task;
  task->setThreadGroup(this);
  for (size_t i=0; i<size(); i++) members[i].start(i, this);
}

inline void ThreadGroup::join()
{
  for(size_t i=0; i<size(); i++) members[i].join();
  running = false;
}

inline void ThreadGroup::main(size_t rank)
{
  if (rank == 0) task->init();
  barrier();
  task->run(rank);
  //barrier();
}


inline void GroupThread::start(size_t rank, ThreadGroup* group)
{
  this->mrank = rank;
  this->mgroup = group;
  pthread_attr_t thread_attr;
  std::memset(&thread_attr, 0, sizeof(thread_attr));
  pthread_attr_init(&thread_attr);
  pthread_attr_setstacksize(&thread_attr, 16*1024);
  pthread_create(&mthread, &thread_attr, &GroupThread::startupHelper, this);
}


inline void GroupThread::startup()
{
  // pin to vcore
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(group()->vcore(rank()), &cpuset);
  pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
  
  // run main method
  group()->main(rank());
}
