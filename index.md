---
layout: page
title: the Many Threads Operating System
---
## Description

The MyThOS project targets at reducing the overhead of the operating system in HPC applications, in order to increase the degree of parallelism and thereby performance. Communication overhead for example is just one constraint – thread instantiation and management, memory management, locking of OS resources etc. are further factors, that are determined by the OS and thereby typically overlooked, because they only recently became obviously relevant with the rise of multicore processors.

Within the MyThOS project the partners are addressing exactly that point: based on real HPC application scenarios a new operating system is being developed, which will reduce the OS factors that affect scalability to a minimum. Traditional operating systems are designed for the “general case” and offer a vast functional palette including user management, security and several I/O-mechanisms. Those functionalities are mostly irrelevant for HPC applications but have an impact on runtime complexity of particular tasks. As opposed to that, within MyThOS each required functionality is implemented in a minimal and modular fashion which allows freely configurable deployment, execution, management and adaptation. Traditional operating systems also strive to establish a higher interactivity and uniform resource distribution between several applications. The requirements of highly-parallel applications are therefor only partially addressed.

MyThOS results in a new distributed OS micro-kernel which is designed and built up from scratch. Fundamental functionalities like thread management, dynamic memory management or communication routines are modularized, optimized for HPC and designed for distributed environments. Through the modular architecture of the kernel new configuration decisions are possible, which can be fine-tuned to fit the specific needs of a concrete application scenario and infrastructure. Special attention is paid to the reduction of thread creation and management costs. Regarding the new lightweight kernel this promises to allow for a far higher parallelization compared to currently established systems.

Reference application scenarios addressed in MyThOS originate from the areas of molecular simulations, computational fluid dynamics and multimedia.

## Sponsored by BMBF
 
![BMBF]({{site.baseurl}}/logo_bmbf.png){:class="img-logoright"}
![DLR]({{site.baseurl}}/logo_dlr.png){:class="img-logoright"}

The MyThOS project was funded by the Federal Ministry of Education and Research (BMBF) under [Grant No. 01/H13003](https://gauss-allianz.de/en/project/title/MyThOS) from October 2013 to September 2016. The grant was part of the 3rd HPC-Call of the Gauss Allianz.

### Partners

* [Universität Ulm, Institut für Organisation und Management von Informationssystemen / Kommunikations & Informationszentrum – Prof. Dr. Stefan Wesner](http://www.uni-ulm.de/in/omi/)
* [Höchstleistungsrechenzentrum Stuttgart – Prof. Dr. Michael Resch](http://www.hlrs.de/)
* [Brandenburgische Technische Universität Cottbus – Prof. Jörg Nolte](http://www.tu-cottbus.de/fakultaet1/de/betriebssysteme/)
* [Alcatel-Lucent Deutschland AG, Bell Labs – Peter Domschitz](https://www.bell-labs.com/)
* [Universität Siegen, Simulationstechnik und Wissenschaftliches Rechnen – Prof. Dr. Sabine Roller](http://www.mb.uni-siegen.de/sts/)
