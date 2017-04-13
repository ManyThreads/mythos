# Deployment: Configuration Management in Modular Software

## Problemstellung
* viele wiederverwendbare, rekombinierbare Softwarekomponenten sind schön
* wie und wo wird konkrete Zusammensetzung festgelegt?
* wie Varianten der Software separat voneinander definieren und erzeugen?

## Grundbegriffe

* run-time configuration: Objekte instantiieren, Variablen initialisieren, Verknüpfen
* compile-time configuration: Konstanten, Typedef, Code-Varianten auswählen
* build management: Was wann wie compilieren und linken

Bsp:
* configure => compile-time configuration
* make => build management

* deklarativ vs. programmatisch, Inferenz von Abhängigkeiten
* forward (what to produce) vs. backward (how to produce)

## Was machen andere?

* Unix V6: Konstanten in include Dateien editieren
* Linux: #define und #ifdef, nur eine feste Zussammensetzung der Software Konfiguration-Mangement durchsetzt kompletten Quellcode
* Java&co: component containers = Beschreibung wie Komponenten instantiieren, konfigurieren und verknüpfen. jar-Pakete und Abhängigkeiten

## Idee 1: Dependency Injection

* run-time configuriation separat von der Implementierung der Komponenten. Alle Konfigurationsdetails und Abhängigkeiten werden von dem magischen Außen geliefert. Benötigte Komponenten nicht selbst erzeugen oder suchen, sondern geben lassen.
* zentrale Deployment Codes: Klebstoff um die Komponenten aufzusetzen und zu verknüpfen (mythos: kernel.cc)
* hat gut funktioniert in Mythos
  * z.B. kontrolliert Komponenten auf bestimmten Kernen instantiieren
  * Wiederverwendung von generischen Komponenten klappt
* aber nicht ausreichend:
  * low-level code braucht Konstanten (Zahlen, Funktionen, Typen), die nicht zur run-time konfiguriert werden können.
  * manche Hardware-spezifische Code-Varianten kann man nicht erst zur run-time auswählen
  * keine Trennung zwischen Konfig des Basissystems und Anwendungs-spezifische Aspekte

## Idee 2: Konfiguration auf 3 Ebenen

1) run-time dependency injection
2) link-time/build-time composition
3) compile-time configuration

* components: run-time config through dependency injection (language objects etc)
* deployment-helpers: encapsulate typical configurations to simplify deployment code (e.g. HWThreadConfig)
* modules: compile-time config (include path, o-file lists, etc)
* meta-modules: encapsulate typical configurations for easier composition

* programmatische Beschreibung reicht uns => Makefile rules
* jede konkrete Konfiguration hat Haupt-Makefile. Dieser inkludiert die "Modulname.mk" der gewünschten Module. Diese liefern make rules, erweitern compiler flags, erweitern datei-listen
* mehrere Module und Modulvarianten im selben Ordner durch entsprechende mk-files

### 3) compile-time configuration über Include Dateien

* Hardware-spezifische include Dateien benutzen ohne kompletten Pfad zu kennen
* Suchpfad des Compilers erweitern oder Konfig. durch symbolische Links / Kopien zusammenstellen

~~~
#include "arch/gdt.h"

cpu/x86-32/include/arch/gdt.h
cpu/x86-64/include/arch/gdt.h

CPPFLAGS += -Icpu/x86-32/include
oder 
CPPFLAGS += -Icpu/x86-64/include
~~~

### 2) build-time composition über Link-Listen

~~~
KERNEL_SRC += cpu/x86-32/gdt.cc
oder
KERNEL_SRC += cpu/gdt-amd64.cc
KERNEL_SRC += cpu/syscall-amd64.S
~~~

* Damit die compilierten o-files verschiedener Konfigurationen nicht in Konflikt kommen, müssen diese in Konfig-spezifischen Verzeichnisbaum abgelegt werden. 
* Dafür müssen die Verzeichnisse existieren => make rules. Bei der Gelegenheit kann man auch gleich alle Source Dateien kopieren/Verlinken.

~~~
TGTFOLDERS += cpu cpu/x86-32
oder
$(TGTDIR)/cpu/gdt-amd64.cc : $(SRCDIR)/cpu/gdt-amd64.cc 
~~~


* object-file Liste im Makefile
* oder durch symbolische Links / Kopien zusammenstellen und Liste generieren

* wo liegt source file und wo der target file?
  * Konflikte zwischen files verschiedener module? => not resolvable for include-files, thus rename and be careful. use subdirs if necessary
  * Konflikte zwischen o-files verschiedener Konfigurationen? => o-file should be generated in config specific path instead of right next to cc-file. => has to generate directory hierarchy in config directory. => could also link files, like done by configure scripts.

XXX can this be achieved from within makefile?

$SRCBASE relative path from makefile to mythos code
$TGTBASE relative path from makefile to config specific files (links/copies, compiled)




###



* möglichst keine Konstanten per -D compiler flag. stattdessen entsprechende komponente mit headerfile
