---
layout: page
title: Documentation
permalink: /doc/
---

## Project Deliverables and Whitepapers

* [D2.2 Gesamtarchitektur]({{site.baseurl}}/doc/D2_2_Gesamtarchitektur.pdf)
* [D2.3 Architekturplan]({{site.baseurl}}/doc/D2_3_Architekturplan.pdf)
* [D3.3 Entwicklungsplan]({{site.baseurl}}/doc/D3_3_Entwicklungsplan.pdf)

* [Software Architectures]({{site.baseurl}}/doc/whitepaper-software-architecture-fundamentals.pdf)
* [Many-Core Architectures]({{site.baseurl}}/doc/whitepaper-many-core-hardware-architectures.pdf)
* [MyThOS Global Architecture]({{site.baseurl}}/doc/whitepaper-global-architecture.pdf) (outdated, a newer version can be found in D2.2 and the project sources)
* [MyThOS Programming Model]({{site.baseurl}}/doc/whitepaper-programming-model.pdf) (slightly outdated and not yet implemented)


## Current State

Many basic functions of modern operating systems are supported by MyThOS, including memory management with support for virtual memory, a dynamic and decentralized management infrastructure for computational resources, dynamic distribution of computational load and strucutres for decentralized communication. The individual components have been carefully designed to provide high scalability and a high degree of parallelism.

## Support for Different Hardware Platforms

During the development of MyThOS a special focus is put on the scalability of the implementation. To evaluate the scalability of different implementation variants an appropriate platform with a high amount of computational resources is required. For this purpose, a Intel XeonPhi Coprocessor card featuring 240 hardware threads is used. On this coprocessor, MyThOS is running as an independent operating system. Equally, MyThOS also supports the amd64 processor architecture and therefore can also be executed on most off-the-shelf desktop and server PCs.
It is intended to port MyThOS to different architectures, as well. Especially architectures for embedded devices, e.g. ARM, will be tageted.

## Future Development

In addition to the already implemented capabilities, MyThOS is to be extended by concepts and methods from real time theory, in order to improve resource allocation, support real time requirements of individual tasks and to provide results with optimal quality. Thereby, MyThOS is intended to support, e.g., interactive simulations, where a user can manipulate the system in real-time. Additionally, MyThOS will be extended with thread migration and distributed shared memory.
