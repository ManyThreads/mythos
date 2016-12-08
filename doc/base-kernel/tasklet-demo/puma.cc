#include "ThreadGroup.hh"
#include "cpuid.hh"
#include <iostream>
#include <mutex>
#include <atomic>

#include <assert.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdio.h>
#include <fcntl.h>
#include <cstdint>
#include "TscClock.h"

std::mutex stdioMutex;
#define IOLOCK(C) { std::unique_lock<std::mutex> _l(stdioMutex); C; }

// RDTSCP: Read 64-bit time-stamp counter and 32-bit IA32_TSC_AUX value into EDX:EAX and ECX.
// Linux encodes numa id (<<12) and core id (8bit) into TSC_AUX
// getcpu - determine CPU and NUMA node on which the calling thread is running


class PUMAMemory
{
public:
  PUMAMemory(size_t size) : datasize(size) {
    pagesize = getpagesize();
    data = (char*)mmap((void*)(10*1024*1024), datasize, PROT_READ | PROT_WRITE,
		MAP_SHARED | MAP_ANONYMOUS | MAP_POPULATE,
		-1, 0);
    // | MAP_HUGETLB | MAP_HUGE_2MB
    if (data == MAP_FAILED) { perror("mmap failed"); exit(-1); }
    auto res = mlock(data, datasize);
    if (res == -1) { perror("mlock failed"); exit(-1); }

    // https://shanetully.com/2014/12/translating-virtual-addresses-to-physcial-addresses-in-user-space/
    mem_fd = open("/proc/self/pagemap", O_RDONLY);
    if (mem_fd == -1) { perror("open pagemap failed"); exit(-1); }

    // for (size_t i=0; i<datasize; i+=4096) {
    //   std::cout << (void*)(data+i) << " -> " << (void*)getpaddr(data+i) << std::endl;
    // }
  }

  ~PUMAMemory() {
    close(mem_fd);
    munmap(data, datasize);
  }

  void* getaddr(size_t offset) const {
    assert(offset<datasize);
    return data+offset;
  }

  size_t getphys(void* ptr) const {
    return getphys(static_cast<char*>(ptr)-data);
  }
  
  size_t getphys(size_t offset) const {
    assert(offset<datasize);
    auto seekres = lseek64(mem_fd, size_t(data+offset)/pagesize*8, SEEK_SET);
    if (seekres == -1) { perror("seeking pagemap failed"); exit(-1); }

    uint64_t desc = 0;
    auto res = read(mem_fd, &desc, sizeof(desc));
    // std::cout << (void*)desc << std::endl;
    if (res == -1) { perror("reading pagemap failed"); exit(-1); }
    assert((desc & (1ull<<63)) != 0); // is present

    size_t pfn = desc & ((1ull<<55)-1);
    return pfn * pagesize + size_t(data+offset)%pagesize;
  }

  size_t getLineIndex(void* ptr) const {
    return getLineIndex(static_cast<char*>(ptr)-data);
  }

  size_t getLineIndex(size_t offset) const { return getphys(offset)/64; }
  size_t size() const { return datasize; }

protected:
  size_t datasize;
  size_t pagesize;
  char* data;
  int mem_fd;
};


struct CacheLine
{
  std::atomic<size_t> comm;
  std::atomic<size_t> minLat;
  std::atomic<size_t> vcore;
};

struct MyTask
  : public BatchSyncTask
{
  MyTask(PUMAMemory* mem)
    : mem(mem)
  {
    for (size_t i=0; i<mem->size()/64; i++) {
      auto a = (CacheLine*)mem->getaddr(i*64);
      a->minLat = -1;
      a->vcore = -1;
    }
  }
  
  virtual ~MyTask() { }

  size_t getCPUThread() const { return mythos::X86cpuid::initialApicID(); }
  
  virtual void init() { iter = 0; }
  virtual bool running() { return iter<mem->size()/64; }
  virtual void step() { iter++; }

  virtual void loop(size_t rank) {
    if (iter==0) {
      IOLOCK(std::cout << " thread " << rank << " of " << numThreads() << " is now running on cpu "
	     << getCPUThread()
	     << std::endl);
    }

    assert(numThreads()%2==0);
    if (rank%2==0) { // master
      auto a = (CacheLine*)mem->getaddr((iter+rank)*64 % mem->size());
      auto b = (CacheLine*)mem->getaddr((iter+rank+1)*64 % mem->size());
      master(a, b);
    } else { // client
      auto a = (CacheLine*)mem->getaddr((iter+rank-1)*64 % mem->size());
      auto b = (CacheLine*)mem->getaddr((iter+rank)*64 % mem->size());
      slave(a, b);
    }      
  }
  
  void master(CacheLine* a, CacheLine* b) {
    TscClock t;
    a->comm = 0;
    size_t olda = 0;
    barrier();

    uint64_t minLat = -1;
    for (int i=0; i<10000; i++) {
      while (a->comm.load(std::memory_order_relaxed) == olda) asm("pause");
      olda++;
      a->comm.fetch_add(0); // get ownership

      t.start();
      b->comm.fetch_add(1);
      t.stop();
      if (t.elapsed() < minLat) minLat = t.elapsed();
    }
    barrier();

    if (minLat < b->minLat) {
      b->minLat = minLat;
      b->vcore = getCPUThread();
    }
      
    IOLOCK(std::cout
	   << getCPUThread()
	   << "; " << (void*)mem->getLineIndex(b)
	   << "; " << minLat << std::endl);
  }

  void slave(CacheLine* a, CacheLine* b) {
    TscClock t;
    b->comm = 0;
    size_t oldb = 0;
    barrier();
    
    uint64_t minLat = -1;
    for (int i=0; i<10000; i++) {
      t.start();
      a->comm.fetch_add(1);
      t.stop();
      if (t.elapsed() < minLat) minLat = t.elapsed();

      while (b->comm.load(std::memory_order_relaxed) == oldb) asm("pause");
      oldb++;
      b->comm.fetch_add(0); // get ownership
    }
    barrier();

    if (minLat < a->minLat) {
      a->minLat = minLat;
      a->vcore = getCPUThread();
    }

    IOLOCK(std::cout
	   << getCPUThread()
	   << "; " << (void*)mem->getLineIndex(a)
	   << "; " << minLat << std::endl);
  }

  void other() {
    barrier();
    barrier();
  }

  PUMAMemory* mem;
  std::atomic<size_t> iter;
};

int main(int argc, char** argv)
{
  PUMAMemory mem(2*4096);
  MyTask t(&mem);
  ThreadGroup gr(2);

  // mapping to logical cores should be driven by policy and scheduling
  //  for (size_t i=0; i<gr.size(); i++) {
  //    gr.setVcore(i, i);
  //  }
  gr.setVcore(0, 0);
  gr.setVcore(1, 4);
  
  gr.start(&t);
  gr.join();
  
  return 0;
}
