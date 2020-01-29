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

* Supports x86-64 aka amd64 on following platforms:
    * emulators like QEMU and Bochs (Gem5 still requires additional patches)
    * as co-kernel next Linux via Riken's IHK on multi-core machines
    * Intel Xeon Phi Knights Corner processor
* Starts an embedded init application on the first core and this
  application can create and configure further kernel objects.
* Creating and starting application threads (aka Execution Contexts)
  on other hardware threads (aka Scheduling Contexts).
* Implemented kernel objects: page maps, frames, capability maps,
  untyped memory for allocation of kernel objects,
  portals for outgoing system calls and messages,
  execution contexts for application threads.
* musl libc, llvm/clang libcxx, llvm libomp integration mostly usable
    * user-mode futex within the same capability space, uses the execution context's binary semaphore for notifications
    * `pthread_create`, `pthread_join`, `pthread_mutex` are working
* x87 FPU support including AVX and AVX512F (needs more testing though)

## Work in Progress

* actual memory management support for mmap
* more complete pthreads and openmp support
* basic benchmarks, testing, performance tuning
* endpoints for receiving incoming messages
* integrating a tracing framework for system-wide performance analysis

## Future Work

* access to the performance counters
* KNC: vector unit support (if needed by users)
* KNC: CPUHOT interrupt and related events (if needed by users)

# Quick Usage Guide

## Running in the QEMU virtual machine

* First, run `git submodule update --init --recursive` to pull the needed projects
* Run `3rdparty/mcconf/install-python-libs.sh` in order to install the build configuration tool. This requires python and pip (a widespread package installer for python).
* Run `3rdparty/install-libcxx.sh` to compile the musl libc and llvm/clang libstdc++
* Now you can run `make` in the root folder. This will assemble the source code into the subfolders `kernel-amd64`, `kernel-knc` and `host-knc`.
* Change into the `kernel-amd64` folder and run `make qemu`. This will compile the init application, the kernel, and finally boot the kernel image inside the qemu emulator. The debug and application output will be written to the console.
* Whenever you add or remove files from modules (the `mcconf.module` files in the `kernel` folder), rerun `make` in the root folder and then `make clean` in the target-specific folder.

## Running on the Intel XeonPhi KNC

In order to run MyThOS on an Intel XeonPhi Knights Corner processor, a recent version of Intel's MPSS software stack is needed. In addition a not too old C++ compiler is needed for the host tools. After loading the respective environment variables:
* First, run `git submodule update --init --recursive` to pull the needed projects
* Run `3rdparty/mcconf/install-python-libs.sh` in order to install the build configuration tool. This requires python and pip (a widespread package installer for python).
* Run `3rdparty/install-libcxx.sh` to compile the musl libc and llvm/clang libstdc++
* Now you can run `make` in the root folder. This will assemble the source code into the subfolders `kernel-amd64`, `kernel-knc` and `host-knc`.
* Change into the `host-knc` folder and run `make` in order to compile the `xmicterm` application. This is used to receive the debugging and log messages from the MyThOS kernel and applications.
* Then, change into the `kernel-knc` folder and run `make` in order to compile the init application and the kernel.
* In order to boot the system on the XeonPhi, you need root access (sudo). Run `make micrun` in order to stop any running coprocessor OS and boot MyThOS. If everything goes well, the script will start the `xmicterm` and you see the debug output. Stop it with CTRL-c and use `make micstop` to shut down the coprocessor. You can enable more detailed debug messages by editing `Makefile.user` and recompiling the kernel.

## Running as IHK cokernel next to CentOS GNU/Linux

* First, run `git submodule update --init --recursive` to pull the needed projects
* Run `3rdparty/mcconf/install-python-libs.sh` in order to install the build configuration tool. This requires python and pip (a widespread package installer for python).
* Run `3rdparty/install-libcxx.sh` to compile the musl libc and llvm/clang libstdc++
* Disable SELinux: `vim /etc/selinux/config` and change the file to SELINUX=disabled
* Reboot the machine: `sudo reboot`
* Install required packages: `sudo yum install cmake kernel-devel binutils-devel systemd-devel numactl-devel`
* Grant read permission to the System.map file of your kernel version: ``sudo chmod a+r /boot/System.map-`uname -r` ``
* Run `3rdparty/install-ihk.sh`
* Now you can run `make` in the root folder. This will assemble the source code into the subfolders `kernel-amd64`, `kernel-ihk`, `kernel-knc` and `host-knc`.
* `cd kernel-ihk`
* `make run`

## Running as IHK cokernel next to Ubuntu GNU/Linux

* `sudo apt install python python-pip python-virtualenv curl cmake`
* `git submodule update --init --recursive` to pull the needed projects.
* Run `3rdparty/mcconf/install-python-libs.sh` in order to install the build configuration tool.
* Run `3rdparty/install-libcxx.sh`
* `sudo apt install linux-headers-$(uname -r) binutils-dev libsystemd-dev libnuma-dev libiberty-dev libudev-dev`
* Perhaps you have to disable AppArmor by entering `sudo systemctl stop apparmor.service` and `sudo update-rc.d -f apparmor remove`
* Grant read permission to the System.map file of your kernel version: ``sudo chmod a+r /boot/System.map-`uname -r` ``
* Run `3rdparty/install-ihk.sh`
* Now you can run `make` in the root folder. This will assemble the source code into the subfolders `kernel-amd64`, `kernel-ihk`, `kernel-knc` and `host-knc`.
* `cd kernel-ihk`
* `make run`

## Problems and Solutions
* If `make qemu` fails with `qemu: could not load PC BIOS 'bios-256k.bin'` on CentOS 7. Fix: `yum install seabios-bin`.
* If `make qemu` fails with `qemu-system-x86_64: Unable to find CPU definition: max`, try to change "-cpu max" into "-cpu SandyBridge" in /mythos/kernel/build/emu-qemu-amd64/mcconf.module

# Acknowledgements

The MyThOS project was funded by the Federal Ministry of Education and Research (BMBF) under Grant No. 01IH13003 from October 2013 to September 2016. The grant was part of the 3rd HPC-Call of the Gauss Allianz.

The high-level kernel design is inspired by the many good ideas from seL4, Nova, Fiasco, and L4. The delegation-based monitors are inspired by delegation locks.
