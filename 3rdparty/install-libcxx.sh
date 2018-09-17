#!/bin/bash

# based on https://wiki.musl-libc.org/building-llvm.html

# need: curl, cmake, libtool, make

# TODO: check libcxxrt (instead of libcxxabi) and compiler-rt (instead of libgcc)


pushd `dirname $0`
BASEDIR=`pwd`
echo installing in $BASEDIR


rm -rf cxx llvm libcxxabi libunwind libcxx

git clone git://git.musl-libc.org/musl

# download stuff
#svn co http://llvm.org/svn/llvm-project/libcxxabi/trunk libcxxabi
curl -C - -LO http://releases.llvm.org/6.0.1/libcxxabi-6.0.1.src.tar.xz
tar -xJf libcxxabi-6.0.1.src.tar.xz
mv libcxxabi-6.0.1.src libcxxabi
#git clone https://github.com/llvm-mirror/libcxxabi.git
curl -C - -LO http://releases.llvm.org/6.0.1/libcxx-6.0.1.src.tar.xz
tar -xJf libcxx-6.0.1.src.tar.xz
mv libcxx-6.0.1.src libcxx
#git clone https://github.com/llvm-mirror/libcxx.git
#git clone https://github.com/llvm-mirror/llvm.git
curl -C - -LO http://releases.llvm.org/6.0.1/libunwind-6.0.1.src.tar.xz
tar -xJf libunwind-6.0.1.src.tar.xz
mv libunwind-6.0.1.src libunwind
curl -C - -LO http://releases.llvm.org/6.0.1/llvm-6.0.1.src.tar.xz
tar -xJf llvm-6.0.1.src.tar.xz
mv llvm-6.0.1.src llvm


cd musl
./configure --prefix="$BASEDIR/cxx"
make -j
make install
cd ..

# install pathscale's libunwind
# git clone https://github.com/pathscale/libunwind.git
# cd libunwind
# ./autogen.sh
# ./configure --prefix="$BASEDIR/cxx"
# make -j
# make install
# cd ..


# install llvm's libunwind
cd libunwind
rm -rf build
mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX:PATH="$BASEDIR/cxx" \
    -DLLVM_PATH="$BASEDIR/llvm" \
    -DCMAKE_BUILD_TYPE=Release \
    ../
make
make install
cd ../..


# install libcxxabi
cd libcxxabi
rm -rf build
mkdir build && cd build
cmake -DLIBCXXABI_LIBCXX_PATH="$BASEDIR/libcxx" \
    -DCMAKE_INSTALL_PREFIX="$BASEDIR/cxx" \
    -DLIBCXXABI_USE_LLVM_UNWINDER=ON \
    -DLIBCXXABI_ENABLE_STATIC_UNWINDER=ON \
    -DLIBCXXABI_ENABLE_NEW_DELETE_DEFINITIONS=OFF \
    -DLLVM_PATH="$BASEDIR/llvm" \
    -DCMAKE_BUILD_TYPE=Release \
    ../
LIBRARY_PATH=$BASEDIR/cxx/lib make -j
make install
cd ../..

# -DLIBUNWIND_CXX_INCLUDE_PATHS
# -DLIBUNWIND_SYSROOT ## for cross-compiling
# -DLIBCXXABI_ENABLE_THREADS=OFF \
# -DLIBCXXABI_BAREMETAL=ON \
# on linux you may need -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++


# install libcxx with libcxxabi
# alternative would be libcxxrt
# see also https://libcxx.llvm.org/docs/BuildingLibcxx.html
cd libcxx
rm -rf build
mkdir build && cd build
cmake -G "Unix Makefiles" \
    -DLIBCXX_CXX_ABI=libcxxabi \
    -DLIBCXX_CXX_ABI_INCLUDE_PATHS="$BASEDIR/libcxxabi/include" \
    -DLIBCXX_CXX_ABI_LIBRARY_PATH="$BASEDIR/cxx/lib" \
    -DLLVM_PATH="$BASEDIR/llvm" \
    -DLIBCXX_ENABLE_STATIC_ABI_LIBRARY=ON \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$BASEDIR/cxx" \
    ../
make -j
make install
cd ../..

#     -DLIBCXX_ENABLE_THREADS=OFF \
#LIBRARY_PATH=$BASEDIR/cxx/lib make -j
#LIBRARY_PATH=$BASEDIR/cxx/lib make install

#    -DLIBCXX_HAS_MUSL_LIBC=ON \


popd
