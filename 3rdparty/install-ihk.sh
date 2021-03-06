#!/bin/bash

SCRIPTDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
IHKDIR="${SCRIPTDIR}/ihk"
BUILDDIR="${IHKDIR}/build"
INSTALLDIR="${IHKDIR}/install"

rm -rf $INSTALLDIR $BUILDDIR

mkdir -p $BUILDDIR
mkdir -p $INSTALLDIR

cd $BUILDDIR
cmake \
  -DENABLE_PERF=OFF \
  -DCMAKE_INSTALL_RPATH="${INSTALLDIR}/lib64/:${INSTALLDIR}/lib/" \
  -DCMAKE_INSTALL_PREFIX=${INSTALLDIR} \
  ${IHKDIR}

make CFLAGS='-DDEBUG_PRINT' install
