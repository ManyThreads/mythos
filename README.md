# About MyThOS

MyThOS is a micro-kernel for many-core processors with cache-coherent
shared memory. Its design aims for high-throughput operation for
highly-dynamic workloads. Examples are task-based high-performance
computing frameworks and service clouds with very fine-grained
application tasks.

Scalability to a high degree of parallelism is achieved as follows:
All kernel objects are allocated explicitly by the application from
freely partitioned memory pools in order to eliminate hidden
synchronisation. Any kernel object can be accessed concurrently from
any core in order to enable parallel system management. Concurrent
operations on kernel objects are serialised through delegation-based
object monitors. Granting and revoking access rights (object
capabilities) affects just a small set of rights in order to minimise
synchronisation.

* https://manythreads.github.io/mythos/

## Current State of Implementation

* Supports x86-64 (aka amd64) emulators like QEMU and Bochs.
  Gem5 still requires additional patches.
* Supports the Intel Xeon Phi Knights Corner processor.
* Starts an embedded init application on the first core and this
  application can create and configure further kernel objects.
* Creating and starting application threads (aka Execution Contexts)
  on other hardware threads (aka Scheduling Contexts).
* Implemented kernel objects: page maps, frames, capability maps,
  untyped memory for allocation of kernel objects,
  portals for outgoing system calls and messages,
  execution contexts for application threads.

## Work in Progress

* basic benchmarks, testing, performance tuning
* endpoints for receiving incoming messages
* asynchronous notifications for Futex and similar constructs
* integrating a tracing framework for system-wide performance analysis
* KNC: floating point and vector unit support
* KNC: CPUHOT interrupt and related events

## Future Work

* access to the performance counters
* improved user-level programming like libc, stdc++

# Quick Usage Guide

## Running in the QEMU virtual machine

* First, run `3rdparty/install-python-libs` in order to install the needed libraries for the build configuration tool. This requires python and pip (a widespread package installer for python).
* Now you can run `make` in the root folder. This will assemble the source code into the subfolders `kernel-amd64`, `kernel-knc` and `host-knc`.
* Change into the `kernel-amd64` folder and run `make qemu`. This will compile the init application, the kernel, and finally boot the kernel image inside the qemu emulator. The debug and application output will be written to the console.
* Whenever you add or remove files from modules (the `mcconf.module` files in the `kernel` folder), rerun `make` in the root folder and then `make clean` in the target-specific folder.

## Running on the Intel XeonPhi KNC

In order to run MyThOS on an Intel XeonPhi Knights Corner processor, a recent version of Intel's MPSS software stack is needed. In addition a not too old C++ compiler is needed for the host tools. After loading the respective environment variables:
* First, run `3rdparty/install-python-libs` in order to install the needed libraries for the build configuration tool. This requires python and pip (a widespread package installer for python).
* Now you can run `make` in the root folder. This will assemble the source code into the subfolders `kernel-amd64`, `kernel-knc` and `host-knc`.
* Change into the `host-knc` folder and run `make` in order to compile the `xmicterm` application. This is used to receive the debugging and log messages from the MyThOS kernel and applications.
* Then, change into the `kernel-knc` folder and run `make` in order to compile the init application and the kernel.
* In order to boot the system on the XeonPhi, you need root access (sudo). Run `make micrun` in order to stop any running coprocessor OS and boot MyThOS. If everything goes well, the script will start the `xmicterm` and you see the debug output. Stop it with CTRL-c and use `make micstop` to shut down the coprocessor. You can enable more detailed debug messages by editing `Makefile.user` and recompiling the kernel.

# Acknowledgements

The MyThOS project was funded by the Federal Ministry of Education and Research (BMBF) under Grant No. 01IH13003 from October 2013 to September 2016. The grant was part of the 3rd HPC-Call of the Gauss Allianz.

The high-level kernel design is inspired by the many good ideas from seL4, Nova, Fiasco, and L4. The delegation-based monitors are inspired by delegation locks.
