#ifndef US_BROADCAST
#define US_BROADCAST

#include "util/SingleLinkedList.hh"

static void broadcast();

extern SingleLinkedList jobPool;

class US_Broadcast{
public:
  US_Broadcast() = default;
  ~US_Broadcast() = default;

  void run(Chain& c);
private:
  void init_broadcast(Chain& c);

  //SingleLinkedList* jobPool;
};

#endif // US_BROADCAST
