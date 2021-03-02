FROM ubuntu:latest

ARG DEBIAN_FRONTEND=noninteractive
RUN apt-get -y update &&\
    apt-get -y upgrade &&\
    apt-get -y install build-essential cmake python virtualenv curl git qemu-system-x86 m4

COPY 3rdparty /usr/src/myapp/3rdparty
WORKDIR /usr/src/myapp

RUN 3rdparty/mcconf/install-python-libs.sh
RUN 3rdparty/install-libcxx.sh

COPY arch-config /usr/src/myapp/arch-config
COPY kernel /usr/src/myapp/kernel
COPY kernel-amd64.config /usr/src/myapp/kernel-amd64.config
COPY Makefile.user /usr/src/myapp/Makefile.user
RUN 3rdparty/mcconf/mcconf -i kernel-amd64.config

WORKDIR /usr/src/myapp/kernel-amd64
RUN make all

CMD ["make", "qemu"]

LABEL Name=mythos Version=0.0.1
