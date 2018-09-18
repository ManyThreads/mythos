#!/bin/bash

# based on https://wiki.musl-libc.org/building-llvm.html

# need: curl, cmake, libtool, make

# TODO: check compiler-rt (instead of libgcc)


### change to basedir
cd `dirname $0`
BASEDIR=`pwd`
echo installing in $BASEDIR
mkdir -p "$BASEDIR/cxx-src"
mkdir -p "$BASEDIR/cxx-amd64"
mkdir -p "$BASEDIR/cxx-knc"

cd cxx-src

### download libraries
if test ! -d musl ; then
  git clone git://git.musl-libc.org/musl
  if test ! $? ; then return $? ; fi
fi

if test ! -d libcxxabi ; then
  #svn co http://llvm.org/svn/llvm-project/libcxxabi/trunk libcxxabi
  #git clone https://github.com/llvm-mirror/libcxxabi.git
  if test ! -e libcxxabi-6.0.1.src.tar.xz ; then  
    curl -C - -LO http://releases.llvm.org/6.0.1/libcxxabi-6.0.1.src.tar.xz
    if test ! $? ; then return $? ; fi
  fi
  tar -xJf libcxxabi-6.0.1.src.tar.xz && mv libcxxabi-6.0.1.src libcxxabi
  if test ! $? ; then return $? ; fi
fi

if test ! -d libcxx ; then
  #git clone https://github.com/llvm-mirror/libcxx.git
  if test ! -e libcxx-6.0.1.src.tar.xz ; then  
    curl -C - -LO http://releases.llvm.org/6.0.1/libcxx-6.0.1.src.tar.xz
    if test ! $? ; then return $? ; fi
  fi
  tar -xJf libcxx-6.0.1.src.tar.xz && mv libcxx-6.0.1.src libcxx
  if test ! $? ; then return $? ; fi
fi

if test ! -d llvm ; then
  #git clone https://github.com/llvm-mirror/llvm.git
  if test ! -e libllvm-6.0.1.src.tar.xz ; then  
    curl -C - -LO http://releases.llvm.org/6.0.1/llvm-6.0.1.src.tar.xz
    if test ! $? ; then return $? ; fi
  fi
  tar -xJf llvm-6.0.1.src.tar.xz && mv llvm-6.0.1.src llvm
  if test ! $? ; then return $? ; fi
fi

if test ! -d libunwind ; then
  if test ! -e libunwind-6.0.1.src.tar.xz ; then  
    curl -C - -LO http://releases.llvm.org/6.0.1/libunwind-6.0.1.src.tar.xz
    if test ! $? ; then return $? ; fi
  fi
  tar -xJf libunwind-6.0.1.src.tar.xz && mv libunwind-6.0.1.src libunwind
  if test ! $? ; then return $? ; fi
fi


### compile for Intel XeonPhi KNC if cross-compiler is present
if test -n `which k1om-mpss-linux-g++` ; then 
  export AS=k1om-mpss-linux-as
  export CXX=k1om-mpss-linux-g++
  export CC=k1om-mpss-linux-gcc
  export LD=k1om-mpss-linux-ld
  export NM=k1om-mpss-linux-nm
  export OBJDUMP=k1om-mpss-linux-objdump
  export STRIP=k1om-mpss-linux-strip

  ### build musl libc for knc
  cd musl
  rm -rf build-knc && mkdir build-knc && cd build-knc
  ../configure --prefix="$BASEDIR/cxx-knc" \
    && make -j && make install
  if test ! $? ; then return $? ; fi
  cd ../..

  ### install llvm's libunwind for knc
  cd libunwind
  rm -rf build-knc && mkdir build-knc && cd build-knc
  cmake -DCMAKE_INSTALL_PREFIX:PATH="$BASEDIR/cxx-knc" \
    -DLLVM_PATH="$BASEDIR/cxx-src/llvm" \
    -DCMAKE_BUILD_TYPE=Release \
    ../ && make && make install
  if test ! $? ; then return $? ; fi
  cd ../..

  ### install libcxxabi for knc
  cd libcxxabi
  rm -rf build-knc && mkdir build-knc && cd build-knc
  cmake -DLIBCXXABI_LIBCXX_PATH="$BASEDIR/cxx-src/libcxx" \
    -DCMAKE_INSTALL_PREFIX="$BASEDIR/cxx-knc" \
    -DLIBCXXABI_USE_LLVM_UNWINDER=ON \
    -DLIBCXXABI_ENABLE_STATIC_UNWINDER=ON \
    -DLIBCXXABI_ENABLE_NEW_DELETE_DEFINITIONS=OFF \
    -DLLVM_PATH="$BASEDIR/cxx-src/llvm" \
    -DCMAKE_BUILD_TYPE=Release \
    ../ \
    && LIBRARY_PATH=$BASEDIR/cxx-knc/lib make -j \
    && make install
  if test ! $? ; then return $? ; fi
  cd ../..

  # install libcxx with libcxxabi for knc
  cd libcxx
  rm -rf build-knc && mkdir build-knc && cd build-knc
  cmake -G "Unix Makefiles" \
    -DLIBCXX_CXX_ABI=libcxxabi \
    -DLIBCXX_CXX_ABI_INCLUDE_PATHS="$BASEDIR/cxx-src/libcxxabi/include" \
    -DLIBCXX_CXX_ABI_LIBRARY_PATH="$BASEDIR/cxx-knc/lib" \
    -DLLVM_PATH="$BASEDIR/cxx-src/llvm" \
    -DLIBCXX_ENABLE_STATIC_ABI_LIBRARY=ON \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$BASEDIR/cxx-knc" \
    ../ && make -j && make install
  if test ! $? ; then return $? ; fi
  cd ../..

fi # done for KNC

echo "all compilation was successfull :)"
exit 0;


### build musl libc for amd64
cd musl
rm -rf build-amd64 && mkdir build-amd64 && cd build-amd64
../configure --prefix="$BASEDIR/cxx-amd64" \
  && make -j && make install
if test ! $? ; then return $? ; fi
cd ../..


### install llvm's libunwind for amd64
cd libunwind
rm -rf build-amd64 && mkdir build-amd64 && cd build-amd64
cmake -DCMAKE_INSTALL_PREFIX:PATH="$BASEDIR/cxx-amd64" \
    -DLLVM_PATH="$BASEDIR/cxx-src/llvm" \
    -DCMAKE_BUILD_TYPE=Release \
    ../ && make && make install
if test ! $? ; then return $? ; fi
cd ../..


### install libcxxabi for amd64
# -DLIBUNWIND_CXX_INCLUDE_PATHS
# -DLIBUNWIND_SYSROOT ## for cross-compiling
# -DLIBCXXABI_ENABLE_THREADS=OFF \
# -DLIBCXXABI_BAREMETAL=ON \
# on linux you may need -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
cd libcxxabi
rm -rf build-amd64 && mkdir build-amd64 && cd build-amd64
cmake -DLIBCXXABI_LIBCXX_PATH="$BASEDIR/cxx-src/libcxx" \
    -DCMAKE_INSTALL_PREFIX="$BASEDIR/cxx-amd64" \
    -DLIBCXXABI_USE_LLVM_UNWINDER=ON \
    -DLIBCXXABI_ENABLE_STATIC_UNWINDER=ON \
    -DLIBCXXABI_ENABLE_NEW_DELETE_DEFINITIONS=OFF \
    -DLLVM_PATH="$BASEDIR/cxx-src/llvm" \
    -DCMAKE_BUILD_TYPE=Release \
    ../ \
  && LIBRARY_PATH=$BASEDIR/cxx-amd64/lib make -j \
  && make install
if test ! $? ; then return $? ; fi
cd ../..


# install libcxx with libcxxabi for amd64
# see also https://libcxx.llvm.org/docs/BuildingLibcxx.html
#     -DLIBCXX_ENABLE_THREADS=OFF 
#LIBRARY_PATH=$BASEDIR/cxx/lib make -j
#LIBRARY_PATH=$BASEDIR/cxx/lib make install
#    -DLIBCXX_HAS_MUSL_LIBC=ON 
cd libcxx
rm -rf build-amd64 && mkdir build-amd64 && cd build-amd64
cmake -G "Unix Makefiles" \
    -DLIBCXX_CXX_ABI=libcxxabi \
    -DLIBCXX_CXX_ABI_INCLUDE_PATHS="$BASEDIR/cxx-src/libcxxabi/include" \
    -DLIBCXX_CXX_ABI_LIBRARY_PATH="$BASEDIR/cxx-amd64/lib" \
    -DLLVM_PATH="$BASEDIR/cxx-src/llvm" \
    -DLIBCXX_ENABLE_STATIC_ABI_LIBRARY=ON \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$BASEDIR/cxx-amd64" \
    ../ && make -j && make install
if test ! $? ; then return $? ; fi
cd ../..

echo "all compilation was successfull :)"

