#pragma once

#include <stdint.h>

// based on http://www.intel.com/content/dam/www/public/us/en/documents/white-papers/ia-32-ia-64-benchmark-code-execution-paper.pdf
// requires feature "rdtscp" in /proc/cpuinfo

#define HAVE_RDTSCP
//#define HAVE_RDTSC_OOO

class TscClock
{
public:
  typedef uint64_t TimeDiff;

  uint32_t start_high;
  uint32_t start_low;
  uint32_t stop_high;
  uint32_t stop_low;
  
#ifdef HAVE_RDTSCP
  inline void startTimer() {
    asm volatile("cpuid\n\t"
		 "rdtsc\n\t"
		 "mov %%edx, %0\n\t"
		 "mov %%eax, %1\n\t"
		 : "=r" (start_high), "=r" (start_low) // timestamp in edx:eax
		 : "a" (0) // eax=1 for cpuid
		 : "%rbx", "%rcx", "%rdx", "memory" // clobbered registers and compiler barrier
		 ); 
  }

  inline void stopTimer() {
    asm volatile("rdtscp\n\t"
		 "mov %%edx, %0\n\t"
		 "mov %%eax, %1\n\t"
		 "cpuid\n\t"
		 : "=r" (stop_high), "=r" (stop_low) // timestamp in edx:eax
		 : // no input
		 : "%rax", "%rbx", "%rcx", "%rdx", "memory" // clobbered registers and compiler barrier
		 ); 
  }
#elif HAVE_RDTSC_OOO
  inline void startTimer() {
    asm volatile("cpuid\n\t"
		 "rdtsc\n\t"
		 "mov %%edx, %0\n\t"
		 "mov %%eax, %1\n\t"
		 : "=r" (start_high), "=r" (start_low) // timestamp in edx:eax
		 : "a" (0) // eax=1 for cpuid
		 : "%rax", "%rbx", "%rcx", "%rdx", "memory" // clobbered registers and compiler barrier
		 );
  }

  inline void stopTimer() {
    asm volatile("cpuid\n\t"
		 "rdtsc\n\t"
		 "mov %%edx, %0\n\t"
		 "mov %%eax, %1\n\t"
		 : "=r" (stop_high), "=r" (stop_low) // timestamp in edx:eax
		 : "a" (0) // eax=1 for cpuid
		 : "%rax", "%rbx", "%rcx", "%rdx", "memory" // clobbered registers and compiler barrier
		 );
  }
#else // INORDER for xeon phi knights corner
  inline void startTimer() {
    asm volatile("rdtsc\n\t"
		 "mov %%edx, %0\n\t"
		 "mov %%eax, %1\n\t"
		 : "=r" (start_high), "=r" (start_low) // timestamp in edx:eax
		 : // no input
		 : "%rax", "%rdx", "memory" // clobbered registers and compiler barrier
		 );
  }

  inline void stopTimer() {
    asm volatile("rdtsc\n\t"
		 "mov %%edx, %0\n\t"
		 "mov %%eax, %1\n\t"
		 : "=r" (stop_high), "=r" (stop_low) // timestamp in edx:eax
		 : // no input
		 : "%rax", "%rdx", "memory" // clobbered registers and compiler barrier
		 );
  }
  
#endif

  void start() { startTimer(); }
  void stop() { stopTimer(); }
  TimeDiff elapsed() const {
    return (TimeDiff(stop_high)<<32 | stop_low) - (TimeDiff(start_high)<<32 | start_low);
  }
  
};
