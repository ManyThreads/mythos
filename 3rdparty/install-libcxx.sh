#!/bin/bash

# based on https://wiki.musl-libc.org/building-llvm.html

function fail {
  echo $1
  exit 1
}


command -v curl >/dev/null 2>&1 || fail "require curl but it's not installed."
command -v cmake >/dev/null 2>&1 || fail "require cmake but it's not installed."
command -v make >/dev/null 2>&1 || fail "require make but it's not installed"
command -v g++ >/dev/null 2>&1 || fail "require g++ but it's not installed"
command -v git >/dev/null 2>&1 || fail "require git but it's not installed"

# need: libtool ???
# TODO: check compiler-rt (instead of libgcc)

command -v k1om-mpss-linux-g++ >/dev/null 2>&1 && CROSS_K1OM=1


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
  git clone git://git.musl-libc.org/musl || fail
  cd musl
  git apply ../../musl-k1om.patch || fail
  cd ..
fi

if test ! -d libcxxabi ; then
  #svn co http://llvm.org/svn/llvm-project/libcxxabi/trunk libcxxabi
  #git clone https://github.com/llvm-mirror/libcxxabi.git
  if test ! -e libcxxabi-6.0.1.src.tar.xz ; then  
    curl -O http://releases.llvm.org/6.0.1/libcxxabi-6.0.1.src.tar.xz || fail
  fi
  tar -xJf libcxxabi-6.0.1.src.tar.xz && mv libcxxabi-6.0.1.src libcxxabi || fail
fi

if test ! -d libcxx ; then
  #git clone https://github.com/llvm-mirror/libcxx.git
  if test ! -e libcxx-6.0.1.src.tar.xz ; then  
    curl -O http://releases.llvm.org/6.0.1/libcxx-6.0.1.src.tar.xz || fail
  fi
  tar -xJf libcxx-6.0.1.src.tar.xz && mv libcxx-6.0.1.src libcxx || fail
fi

if test ! -d llvm ; then
  #git clone https://github.com/llvm-mirror/llvm.git
  if test ! -e libllvm-6.0.1.src.tar.xz ; then  
    curl -O http://releases.llvm.org/6.0.1/llvm-6.0.1.src.tar.xz || fail
  fi
  tar -xJf llvm-6.0.1.src.tar.xz && mv llvm-6.0.1.src llvm || fail
fi

if test ! -d libunwind ; then
  if test ! -e libunwind-6.0.1.src.tar.xz ; then  
    curl -O http://releases.llvm.org/6.0.1/libunwind-6.0.1.src.tar.xz || fail
  fi
  tar -xJf libunwind-6.0.1.src.tar.xz && mv libunwind-6.0.1.src libunwind || fail
fi



### build musl libc for amd64
# don't try parallel builds
cd musl
rm -rf build-amd64 && mkdir build-amd64 && cd build-amd64
env CXXFLAGS="-nostdinc" \
../configure --prefix="$BASEDIR/cxx-amd64" \
  && make && make install || fail
cd ../..

### build preliminary libcxx just for the header files
cd libcxx
rm -rf build-amd64 && mkdir build-amd64 && cd build-amd64
env CXXFLAGS="-nostdinc -I$BASEDIR/cxx-amd64/include" \
cmake -G "Unix Makefiles" \
    -DLLVM_PATH="$BASEDIR/cxx-src/llvm" \
    -DCMAKE_INSTALL_PREFIX="$BASEDIR/cxx-amd64" \
    ../ && make -j && make install || fail
cd ../..

### install llvm's libunwind for amd64
cd libunwind
rm -rf build-amd64 && mkdir build-amd64 && cd build-amd64
env CXXFLAGS="-nostdinc -I$BASEDIR/cxx-amd64/include/c++/v1 -I$BASEDIR/cxx-amd64/include" \
cmake -DCMAKE_INSTALL_PREFIX:PATH="$BASEDIR/cxx-amd64" \
    -DLLVM_PATH="$BASEDIR/cxx-src/llvm" \
    -DCMAKE_BUILD_TYPE=Release \
    ../ && make && make install || fail
cd ../..


### install libcxxabi for amd64
# -DLIBUNWIND_CXX_INCLUDE_PATHS
# -DLIBUNWIND_SYSROOT ## for cross-compiling
# -DLIBCXXABI_ENABLE_THREADS=OFF \
# -DLIBCXXABI_BAREMETAL=ON \
# on linux you may need -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
cd libcxxabi
rm -rf build-amd64 && mkdir build-amd64 && cd build-amd64
env CXXFLAGS="-nostdinc -I$BASEDIR/cxx-amd64/include -I$BASEDIR/cxx-amd64/include/c++/v1 -I$BASEDIR/cxx-amd64/include" \
cmake -DLIBCXXABI_LIBCXX_PATH="$BASEDIR/cxx-src/libcxx" \
    -DCMAKE_INSTALL_PREFIX="$BASEDIR/cxx-amd64" \
    -DLIBCXXABI_USE_LLVM_UNWINDER=ON \
    -DLIBCXXABI_LIBUNWIND_INCLUDES="$BASEDIR/cxx-src/libunwind/include" \
    -DLIBCXXABI_ENABLE_STATIC_UNWINDER=ON \
    -DLIBCXXABI_ENABLE_NEW_DELETE_DEFINITIONS=OFF \
    -DLLVM_PATH="$BASEDIR/cxx-src/llvm" \
    -DLIBCXXABI_SHARED_LINK_FLAGS="-L$BASEDIR/cxx-amd64/lib" \
    -DCMAKE_BUILD_TYPE=Release \
    ../ && make -j && make install || fail
cd ../..


# install libcxx with libcxxabi for amd64
# see also https://libcxx.llvm.org/docs/BuildingLibcxx.html
#     -DLIBCXX_ENABLE_THREADS=OFF 
#LIBRARY_PATH=$BASEDIR/cxx/lib make -j
#LIBRARY_PATH=$BASEDIR/cxx/lib make install
#    -DLIBCXX_HAS_MUSL_LIBC=ON 
cd libcxx
rm -rf build-amd64 && mkdir build-amd64 && cd build-amd64
env CXXFLAGS="-nostdinc -I$BASEDIR/cxx-amd64/include -I$BASEDIR/cxx-amd64/include/c++/v1 -I$BASEDIR/cxx-amd64/include" \
cmake -G "Unix Makefiles" \
    -DLIBCXX_CXX_ABI=libcxxabi \
    -DLIBCXX_CXX_ABI_INCLUDE_PATHS="$BASEDIR/cxx-src/libcxxabi/include" \
    -DLIBCXX_CXX_ABI_LIBRARY_PATH="$BASEDIR/cxx-amd64/lib" \
    -DLLVM_PATH="$BASEDIR/cxx-src/llvm" \
    -DLIBCXX_ENABLE_STATIC_ABI_LIBRARY=ON \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$BASEDIR/cxx-amd64" \
    ../ && make -j && make install || fail
cd ../..


### compile for Intel XeonPhi KNC if cross-compiler is present
if test f$CROSS_K1OM = f1 ; then 
  export AS=k1om-mpss-linux-as
  export CXX=k1om-mpss-linux-g++
  export CC=k1om-mpss-linux-gcc
  export LD=k1om-mpss-linux-ld
  export NM=k1om-mpss-linux-nm
  export OBJDUMP=k1om-mpss-linux-objdump
  export STRIP=k1om-mpss-linux-strip

  ### build musl libc for knc
  # don't do parallel builds!
  cd musl
  rm -rf build-knc && mkdir build-knc && cd build-knc
  ../configure --target=k1om-mpss-linux --prefix="$BASEDIR/cxx-knc" \
    && make && make install || fail
  cd ../..

  ### install llvm's libunwind for knc
  cd libunwind
  rm -rf build-knc && mkdir build-knc && cd build-knc
  cmake -DCMAKE_INSTALL_PREFIX="$BASEDIR/cxx-knc" \
    -DLLVM_PATH="$BASEDIR/cxx-src/llvm" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_COMPILER=k1om-mpss-linux-g++ \
    -DCMAKE_C_COMPILER=k1om-mpss-linux-gcc \
    ../ && make && make install || fail
  cd ../..

  ### install libcxxabi for knc
  # -DLIBCXXABI_ENABLE_STATIC_UNWINDER=ON \
  cd libcxxabi
  rm -rf build-knc && mkdir build-knc && cd build-knc
  cmake -DLIBCXXABI_LIBCXX_PATH="$BASEDIR/cxx-src/libcxx" \
    -DCMAKE_INSTALL_PREFIX="$BASEDIR/cxx-knc" \
    -DLIBCXXABI_USE_LLVM_UNWINDER=ON \
    -DLIBCXXABI_ENABLE_STATIC_UNWINDER=ON \
    -DLIBCXXABI_ENABLE_NEW_DELETE_DEFINITIONS=OFF \
    -DLLVM_PATH="$BASEDIR/cxx-src/llvm" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_COMPILER=k1om-mpss-linux-g++ \
    -DCMAKE_C_COMPILER=k1om-mpss-linux-gcc \
    -DLIBCXXABI_SHARED_LINK_FLAGS="-L$BASEDIR/cxx-knc/lib" \
    ../ && make -j && make install || fail
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
    -DCMAKE_CXX_COMPILER=k1om-mpss-linux-g++ \
    -DCMAKE_C_COMPILER=k1om-mpss-linux-gcc \
    -DCMAKE_INSTALL_PREFIX="$BASEDIR/cxx-knc" \
    ../ && make -j && make install || fail
  cd ../..

else
  echo "skipping K1OM/KNC because cross-compiler k1om-mpss-linux-g++ not found."

fi # done for KNC

echo "all compilation was successfull :)"

