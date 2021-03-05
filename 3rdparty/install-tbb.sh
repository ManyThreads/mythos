#!/bin/bash

SCRIPTDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
TBBDIR="${SCRIPTDIR}/tbb"
CXXDIR="${SCRIPTDIR}/cxx-amd64"
TBB_BUILD_PREFIX="my_tbb"

MYFLAGS=" -std=c++11 -march=native -Wfatal-errors -g"
MYFLAGS+=" -fno-stack-protector"
MYFLAGS+=" -nostdlib -nostdinc -nostdinc++" 
MYFLAGS+=" -isystem ${CXXDIR}/usr/include/c++/v1"
MYFLAGS+=" -isystem ${CXXDIR}/usr/include"

export CXXFLAGS="${MYFLAGS} ${CPLUS_FLAGS}"
export tbb_build_prefix="${TBB_BUILD_PREFIX}"

echo $CPLUS_FLAGS

cd $TBBDIR
make clean default
make extra_inc=big_iron.inc
cd -
