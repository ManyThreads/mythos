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
mkdir -p "$DSTDIR"
mkdir -p "$BASEDIR/cxx-knc"

cd cxx-src

### download libraries
if test ! -d musl ; then
  git clone git://git.musl-libc.org/musl || fail
  cd musl
  git apply ../../musl-k1om.patch || fail
  git apply ../../musl-strtoll_l.patch || fail
  # printf_chk based on https://www.openwall.com/lists/musl/2015/06/20/5
  git apply ../../musl-printf_chk.patch || fail
  cd ..
fi

if test ! -d libcxxabi ; then
  #svn co http://llvm.org/svn/llvm-project/libcxxabi/trunk libcxxabi
  #git clone https://github.com/llvm-mirror/libcxxabi.git
  if test ! -e libcxxabi-6.0.1.src.tar.xz ; then  
    curl -O http://releases.llvm.org/6.0.1/libcxxabi-6.0.1.src.tar.xz || fail
  fi
  tar -xJf libcxxabi-6.0.1.src.tar.xz && mv libcxxabi-6.0.1.src libcxxabi || fail
  cd libcxxabi
  #patch -p1 < ../../libcxxabi-cmake.patch
  cd ..
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


function compile {

### build musl libc for amd64
# don't try parallel builds
cd musl
rm -rf build && mkdir build && cd build
env CXXFLAGS="-nostdinc" \
../configure $MUSLTARGET --prefix="$DSTDIR/usr" \
  && make && make install || fail
cd ../..

### build preliminary libcxx just for the header files
#    -DCMAKE_CC_COMPILER="$DSTDIR/usr/bin/musl-gcc"
cd libcxx
rm -rf build && mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX="$DSTDIR/usr" \
    -DCMAKE_C_COMPILER="$DSTDIR/usr/bin/musl-gcc" \
    -DCMAKE_CXX_COMPILER="$DSTDIR/usr/bin/musl-gcc" \
    -DCMAKE_CXX_FLAGS="-isystem $DSTDIR/usr/include -isystem $DSTDIR/usr/include/c++/v1" \
    -DLLVM_PATH="$BASEDIR/cxx-src/llvm" \
    ../ && make -j && make install || fail
cd ../..

### install llvm's libunwind for amd64
cd libunwind
rm -rf build && mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX="$DSTDIR/usr" \
    -DCMAKE_C_COMPILER="$DSTDIR/usr/bin/musl-gcc" \
    -DCMAKE_CXX_COMPILER="$DSTDIR/usr/bin/musl-gcc" \
    -DCMAKE_CXX_FLAGS="-isystem $DSTDIR/usr/include -isystem $DSTDIR/usr/include/c++/v1" \
    -DCMAKE_BUILD_TYPE=Release \
    -DLLVM_PATH="$BASEDIR/cxx-src/llvm" \
    ../ && make && make install || fail
cd ../..


### install libcxxabi for amd64
# -DLIBUNWIND_SYSROOT ## for cross-compiling
# -DLIBCXXABI_ENABLE_THREADS=OFF \
# -DLIBCXXABI_BAREMETAL=ON \
# on linux you may need -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
cd libcxxabi
rm -rf build && mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX="$DSTDIR/usr" \
    -DCMAKE_C_COMPILER="$DSTDIR/usr/bin/musl-gcc" \
    -DCMAKE_CXX_COMPILER="$DSTDIR/usr/bin/musl-gcc" \
    -DCMAKE_CXX_FLAGS="-isystem $DSTDIR/usr/include -isystem $DSTDIR/usr/include/c++/v1" \
    -DLIBCXXABI_SYSROOT="$DSTDIR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DLIBCXXABI_LIBCXX_PATH="$BASEDIR/cxx-src/libcxx" \
    -DLIBCXXABI_USE_LLVM_UNWINDER=ON \
    -DLIBCXXABI_LIBUNWIND_INCLUDES="$BASEDIR/cxx-src/libunwind/include" \
    -DLIBCXXABI_ENABLE_STATIC_UNWINDER=ON \
    -DLIBCXXABI_ENABLE_NEW_DELETE_DEFINITIONS=OFF \
    -DLIBCXXABI_SHARED_LINK_FLAGS="-L$DSTDIR/usr/lib" \
    -DLLVM_PATH="$BASEDIR/cxx-src/llvm" \
    ../ && make -j && make install || fail
cd ../..


# install libcxx with libcxxabi for amd64
# see also https://libcxx.llvm.org/docs/BuildingLibcxx.html
#     -DLIBCXX_ENABLE_THREADS=OFF 
#LIBRARY_PATH=$BASEDIR/cxx/lib make -j
#LIBRARY_PATH=$BASEDIR/cxx/lib make install
#    -DLIBCXX_HAS_MUSL_LIBC=ON 
cd libcxx
rm -rf build && mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX="$DSTDIR/usr" \
    -DCMAKE_C_COMPILER="$DSTDIR/usr/bin/musl-gcc" \
    -DCMAKE_CXX_COMPILER="$DSTDIR/usr/bin/musl-gcc" \
    -DCMAKE_CXX_FLAGS="-isystem $DSTDIR/usr/include -isystem $DSTDIR/usr/include/c++/v1" \
    -DCMAKE_BUILD_TYPE=Release \
    -DLIBCXX_CXX_ABI=libcxxabi \
    -DLIBCXX_CXX_ABI_INCLUDE_PATHS="$BASEDIR/cxx-src/libcxxabi/include" \
    -DLIBCXX_CXX_ABI_LIBRARY_PATH="$DSTDIR/usr/lib" \
    -DLLVM_PATH="$BASEDIR/cxx-src/llvm" \
    -DLIBCXX_ENABLE_STATIC_ABI_LIBRARY=ON \
    ../ && make -j && make install || fail
cd ../..

} # function compile



export DSTDIR="$BASEDIR/cxx-amd64"
export MUSLTARGET=""
#export REALGCC=
compile

### compile for Intel XeonPhi KNC if cross-compiler is present
if test f$CROSS_K1OM = f1 ; then 
  export DSTDIR="$BASEDIR/cxx-knc"
  export MUSLTARGET="--target=k1om-mpss-linux"
  export REALGCC="k1om-mpss-linux-gcc"
  export CC="k1om-mpss-linux-gcc"
  export CXX="k1om-mpss-linux-g++"
  compile
else
  echo "skipping K1OM/KNC because cross-compiler k1om-mpss-linux-g++ not found."
fi # done for KNC

echo "all compilation was successfull :)"
