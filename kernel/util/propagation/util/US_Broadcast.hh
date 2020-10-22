#ifndef US_BROADCAST
#define US_BROADCAST

#include "util/SingleLinkedList.hh"

template <typename T>
class US_Broadcast{
public:
  US_Broadcast();
  US_Broadcast(SingleLinkedList<T>* jobs);
  ~US_BROADCAST();

  void init(SingleLinkedList<T>* jobs);
  void run(Chain<T>* c);
private:
  void init_broadcast()
  void broadcast(Chain<T>* c, std::atomic<int> res);

  SingleLinkedList<T>* jobs;
}

#endif // US_BROADCAST
