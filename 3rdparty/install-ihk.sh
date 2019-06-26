#!/bin/bash

SCRIPTDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
IHKDIR="${SCRIPTDIR}/ihk"
BUILDDIR="${IHKDIR}/build"
INSTALLDIR="${IHKDIR}/install"

rm -rf $INSTALLDIR/*

mkdir -p $BUILDDIR
mkdir -p $INSTALLDIR

cd $BUILDDIR
cmake -DCMAKE_INSTALL_RPATH="${INSTALLDIR}/lib64/" -DCMAKE_INSTALL_PREFIX=${INSTALLDIR} ${IHKDIR}
make install

