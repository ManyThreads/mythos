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

export CMAKE=cmake
command -v cmake3 >/dev/null 2>&1 && CMAKE=cmake3

command -v k1om-mpss-linux-g++ >/dev/null 2>&1 && CROSS_K1OM=1


### change to basedir
cd `dirname $0`
BASEDIR=`pwd`
echo installing in $BASEDIR

#git submodule update --init --recursive

mkdir cxx-src
cd cxx-src

### download libraries
rm -rf musl
ln -s ../musl

#svn co http://llvm.org/svn/llvm-project/libcxxabi/trunk libcxxabi
#git clone https://github.com/llvm-mirror/libcxxabi.git
if test ! -e libcxxabi-9.0.0.src.tar.xz ; then  
  curl -LO http://releases.llvm.org/9.0.0/libcxxabi-9.0.0.src.tar.xz || fail
fi
rm -rf libcxxabi
tar -xJf libcxxabi-9.0.0.src.tar.xz && mv libcxxabi-9.0.0.src libcxxabi || fail

#git clone https://github.com/llvm-mirror/libcxx.git
if test ! -e libcxx-9.0.0.src.tar.xz ; then  
  curl -LO http://releases.llvm.org/9.0.0/libcxx-9.0.0.src.tar.xz || fail
fi
rm -rf libcxx
tar -xJf libcxx-9.0.0.src.tar.xz && mv libcxx-9.0.0.src libcxx || fail
cd libcxx
patch -p1 < ../../libcxx-nolinux.patch
patch -p1 < ../../libcxx-musl-compat.patch
cd ..

#git clone https://github.com/llvm-mirror/llvm.git
if test ! -e llvm-9.0.0.src.tar.xz ; then  
  curl -LO http://releases.llvm.org/9.0.0/llvm-9.0.0.src.tar.xz || fail
fi
rm -rf llvm
tar -xJf llvm-9.0.0.src.tar.xz && mv llvm-9.0.0.src llvm || fail

if test ! -e libunwind-9.0.0.src.tar.xz ; then  
  curl -LO http://releases.llvm.org/9.0.0/libunwind-9.0.0.src.tar.xz || fail
fi
rm -rf libunwind
tar -xJf libunwind-9.0.0.src.tar.xz && mv libunwind-9.0.0.src libunwind || fail

#openmp
if test ! -e openmp-9.0.0.src.tar.xz ; then  
  curl -LO http://releases.llvm.org/9.0.0/openmp-9.0.0.src.tar.xz || fail
fi
#rm -rf openmp
#tar -xJf openmp-9.0.0.src.tar.xz && mv openmp-9.0.0.src openmp || fail



###########################################################
function compile {

rm -rf "$DSTDIR"
mkdir -p "$DSTDIR"

### build musl libc for amd64
# don't try parallel builds
# need to define mythos-platform as derivate of k1om and x86-64
# need to define SYSCALL_NO_INLINE and just call external function mythos_syscall
# then dispatch the few syscalls we are actually interested in
# also define a new thread/x86-64/syscall_cp.s and clone.s, set_thread_area and unmapself
cd musl
rm -rf build && mkdir build && cd build
env CC=$REALGCC CFLAGS="-nostdinc" \
../configure $MUSLTARGET --prefix="$DSTDIR/usr" \
    --disable-shared \
    --enable-wrapper=all \
    && make -j`nproc` && make install || fail
cd ../..

### build preliminary libcxx just for the header files
#    -DCMAKE_CC_COMPILER="$DSTDIR/usr/bin/musl-gcc"
cd libcxx
rm -rf build && mkdir build && cd build
$CMAKE -DCMAKE_INSTALL_PREFIX="$DSTDIR/usr" \
    -DLIBCXX_HAS_MUSL_LIBC=ON \
    -DCMAKE_FIND_ROOT_PATH="$DSTDIR" \
    -DCMAKE_C_COMPILER="$DSTDIR/usr/bin/musl-gcc" \
    -DCMAKE_CXX_COMPILER="$DSTDIR/usr/bin/musl-gcc" \
    -DCMAKE_CXX_FLAGS="-isystem $DSTDIR/usr/include -isystem $DSTDIR/usr/include/c++/v1" \
    -DCMAKE_BUILD_TYPE=Release \
    -DLIBCXX_SYSROOT="$DSTDIR" \
    -DLIBCXX_ENABLE_SHARED=OFF \
    -DLLVM_PATH="$BASEDIR/cxx-src/llvm" \
    ../ && make -j`nproc` && make install || fail
cd ../..

### install llvm's libunwind for amd64
cd libunwind
rm -rf build && mkdir build && cd build
$CMAKE -DCMAKE_INSTALL_PREFIX="$DSTDIR/usr" \
    -DCMAKE_C_COMPILER="$DSTDIR/usr/bin/musl-gcc" \
    -DCMAKE_CXX_COMPILER="$DSTDIR/usr/bin/musl-gcc" \
    -DCMAKE_CXX_FLAGS="-isystem $DSTDIR/usr/include -isystem $DSTDIR/usr/include/c++/v1" \
    -DLIBUNWIND_SYSROOT="$DSTDIR" \
    -DLIBUNWIND_ENABLE_SHARED=OFF \
    -DCMAKE_BUILD_TYPE=Release \
    -DLLVM_PATH="$BASEDIR/cxx-src/llvm" \
    ../ && make -j`nproc` && make install || fail
cd ../..


### install libcxxabi for amd64
# -DLIBCXXABI_ENABLE_THREADS=OFF would also disable c++11 atomics :(
# -DLIBCXXABI_BAREMETAL=ON 
# on linux you may need -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
#    -DLIBCXXABI_USE_LLVM_UNWINDER=ON \
#    -DLIBCXXABI_LIBUNWIND_INCLUDES="$BASEDIR/cxx-src/libunwind/include" \
#    -DLIBCXXABI_ENABLE_STATIC_UNWINDER=ON \
cd libcxxabi
rm -rf build && mkdir build && cd build
$CMAKE -DCMAKE_INSTALL_PREFIX="$DSTDIR/usr" \
    -DCMAKE_C_COMPILER="$DSTDIR/usr/bin/musl-gcc" \
    -DCMAKE_CXX_COMPILER="$DSTDIR/usr/bin/musl-gcc" \
    -DCMAKE_CXX_FLAGS="-isystem $DSTDIR/usr/include -isystem $DSTDIR/usr/include/c++/v1" \
    -DCMAKE_BUILD_TYPE=Release \
    -DLIBCXXABI_SYSROOT="$DSTDIR" \
    -DLIBCXXABI_ENABLE_SHARED=OFF \
    -DLIBCXXABI_LIBCXX_PATH="$BASEDIR/cxx-src/libcxx" \
    -DLIBCXXABI_ENABLE_NEW_DELETE_DEFINITIONS=OFF \
    -DLIBCXXABI_SHARED_LINK_FLAGS="-L$DSTDIR/usr/lib" \
    -DLLVM_PATH="$BASEDIR/cxx-src/llvm" \
    ../ && make -j`nproc` && make install || fail
cd ../..


# install libcxx with libcxxabi for amd64
# see also https://libcxx.llvm.org/docs/BuildingLibcxx.html
#LIBRARY_PATH=$BASEDIR/cxx/lib make -j
#LIBRARY_PATH=$BASEDIR/cxx/lib make install
cd libcxx
rm -rf build && mkdir build && cd build
$CMAKE -DCMAKE_INSTALL_PREFIX="$DSTDIR/usr" \
    -DLIBCXX_HAS_MUSL_LIBC=ON \
    -DCMAKE_C_COMPILER="$DSTDIR/usr/bin/musl-gcc" \
    -DCMAKE_CXX_COMPILER="$DSTDIR/usr/bin/musl-gcc" \
    -DCMAKE_CXX_FLAGS="-isystem $DSTDIR/usr/include -isystem $DSTDIR/usr/include/c++/v1" \
    -DCMAKE_BUILD_TYPE=Release \
    -DLIBCXX_SYSROOT="$DSTDIR" \
    -DLIBCXX_ENABLE_SHARED=OFF \
    -DLIBCXX_CXX_ABI=libcxxabi \
    -DLIBCXX_CXX_ABI_INCLUDE_PATHS="$BASEDIR/cxx-src/libcxxabi/include" \
    -DLIBCXX_CXX_ABI_LIBRARY_PATH="$DSTDIR/usr/lib" \
    -DLLVM_PATH="$BASEDIR/cxx-src/llvm" \
    -DLIBCXX_ENABLE_STATIC_ABI_LIBRARY=ON \
    ../ && make -j`nproc` && make install || fail
cd ../..


# install openmp
#    -DLIBOMP_SYSROOT="$DSTDIR"
cd openmp
rm -rf build && mkdir build && cd build
$CMAKE -DCMAKE_INSTALL_PREFIX="$DSTDIR/usr" \
    -DOPENMP_ENABLE_LIBOMPTARGET=OFF \
    -DCMAKE_C_COMPILER="$DSTDIR/usr/bin/musl-gcc" \
    -DCMAKE_CXX_COMPILER="$DSTDIR/usr/bin/musl-gcc" \
    -DCMAKE_CXX_FLAGS="-isystem $DSTDIR/usr/include -isystem $DSTDIR/usr/include/c++/v1" \
    -DCMAKE_BUILD_TYPE=Release \
    -DLIBOMP_ENABLE_SHARED=OFF \
    ../ && make -j`nproc` && make install || fail
cd ../..

} # function compile
###########################################################



export DSTDIR="$BASEDIR/cxx-amd64"
export MUSLTARGET=""
#export REALGCC=
compile

### compile for Intel XeonPhi KNC if cross-compiler is present
if test f$CROSS_K1OM = f1 ; then 
  export DSTDIR="$BASEDIR/cxx-knc"
  export MUSLTARGET="--target=k1om-mpss-linux"
  export REALGCC="k1om-mpss-linux-gcc"
  compile
  cp -v `k1om-mpss-linux-gcc --print-sysroot`/usr/lib64/k1om-mpss-linux/5.1.1/libgcc.a $DSTDIR/usr/lib/
else
  echo "skipping K1OM/KNC because cross-compiler k1om-mpss-linux-g++ not found."
fi # done for KNC

echo "all compilation was successfull :)"
