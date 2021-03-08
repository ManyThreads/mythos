#ifndef US_BROADCAST
#define US_BROADCAST

#include "util/SingleLinkedList.hh"

extern SingleLinkedList jobPool;

void* run(Chain& c);
void init_broadcast(Chain& c);
void* broadcast(void* args);

#endif // US_BROADCAST
