#!/bin/bash

SCRIPTDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
IHKDIR="${SCRIPTDIR}/ihk"
BUILDDIR="${IHKDIR}/build"
INSTALLDIR="${IHKDIR}/install"

cp "${SCRIPTDIR}/ihk-templates/ihkmond.c" "${IHKDIR}/linux/user/"
cp "${SCRIPTDIR}/ihk-templates/mikc.c" "${IHKDIR}/linux/core/"
cp "${SCRIPTDIR}/ihk-templates/smp-arch-driver.c" "${IHKDIR}/linux/driver/smp/arch/x86_64/"


rm -rf $INSTALLDIR $BUILDDIR

mkdir -p $BUILDDIR
mkdir -p $INSTALLDIR

cd $BUILDDIR
cmake -DENABLE_PERF=OFF -DCMAKE_INSTALL_RPATH="${INSTALLDIR}/lib64/:${INSTALLDIR}/lib/" -DCMAKE_INSTALL_PREFIX=${INSTALLDIR} -DCMAKE_CXXFLAGS='-DDEBUG_PRINT' ${IHKDIR}

make install

