/* -*- mode:C++; -*- */
#pragma once

#include "Tasklet.h"
#include <mutex>
#include <iostream>

std::mutex stdioMutex;
#define IOLOCK(C) { std::unique_lock<std::mutex> _l(stdioMutex); C; }

extern thread_local size_t threadID;

namespace trace {

  void recv(Tasklet* m, void* target, void* cont, char const* method) {
    std::unique_lock<std::mutex> _l(stdioMutex);
    std::cout << threadID << ": send"
	      << "\t" << method
	      << "\t" << target
	      << "\t" << m
	      << "\t" << cont << std::endl;
  }

  void recv(Tasklet* m, void* target, char const* method) {
    std::unique_lock<std::mutex> _l(stdioMutex);
    std::cout << threadID << ": send"
	      << "\t" << method
	      << "\t" << target
	      << "\t" << m 
	      << "\t-" << std::endl;
  }
  
  void begin(Tasklet* m, void* target, void* cont, char const* method) {
    std::unique_lock<std::mutex> _l(stdioMutex);
    std::cout << threadID << ": exec"
	      << "\t" << method
      	      << "\t" << target
	      << "\t" << m
      	      << "\t" << cont << std::endl;
  }

  void begin(Tasklet* m, void* target, char const* method) {
    std::unique_lock<std::mutex> _l(stdioMutex);
    std::cout << threadID << ": exec"
	      << "\t" << method
	      << "\t" << m
      	      << "\t" << target
	      << "\t-" << std::endl;
  }

  void nestBegin() {
    std::unique_lock<std::mutex> _l(stdioMutex);
    std::cout << threadID << ": nested begin"
	      << "\t-\t-\t-\t-" << std::endl;
  }

  void nestEnd() {
    std::unique_lock<std::mutex> _l(stdioMutex);
    std::cout << threadID << ": nested end"
	      << "\t-\t-\t-\t-" << std::endl;
  }
  
}
