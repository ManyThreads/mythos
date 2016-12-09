# About MyThOS

MyThOS is a micro-kernel for shared-memory many-core processors.

Many basic functions of modern operating systems are supported by MyThOS, including memory management with support for virtual memory, a dynamic and decentralized management infrastructure for computational resources, dynamic distribution of computational load and strucutres for decentralized communication. The individual components have been carefully designed to provide high scalability and a high degree of parallelism.

# Compiling Quick Guide

* First, run `3rdparty/install-python-libs` in order to install the needed libraries for the build configuration tool. This requires python and pip (a widespread package installer for python).
* Now you can run `make` in the root folder. This will assemble the source code into the subfolders `kernel-amd64`, `kernel-knc` and `host-knc`.
* Change into the `kernel-amd64` folder and run `make qemu`. This will compile the init application, the kernel, and finally boot the kernel image inside the qemu emulator. The debug and application output will be written to the console.
* Whenever you add or remove files from modules (the `mcconf.module` files in the `kernel` folder), rerun `make` in the root folder and then `make clean all` in the target-specific folder.
