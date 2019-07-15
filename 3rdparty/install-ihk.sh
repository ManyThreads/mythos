#!/bin/bash

SCRIPTDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
IHKDIR="${SCRIPTDIR}/ihk"
BUILDDIR="${IHKDIR}/build"
INSTALLDIR="${IHKDIR}/install"

cp "${SCRIPTDIR}/ihk-templates/ihkmond.c" "${IHKDIR}/linux/user/"
cp "${SCRIPTDIR}/ihk-templates/mikc.c" "${IHKDIR}/linux/core/"

rm -rf $INSTALLDIR $BUILDDIR

mkdir -p $BUILDDIR
mkdir -p $INSTALLDIR

cd $BUILDDIR
cmake -DCMAKE_INSTALL_RPATH="${INSTALLDIR}/lib64/" -DCMAKE_INSTALL_PREFIX=${INSTALLDIR} -DCMAKE_CXXFLAGS='-DDEBUG_PRINT' ${IHKDIR}
make install

