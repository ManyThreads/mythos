FROM ubuntu:latest

ARG DEBIAN_FRONTEND=noninteractive
RUN apt -y update &&\
    apt -y upgrade &&\
    apt -y install build-essential cmake python virtualenv curl git qemu-system-x86

COPY 3rdparty /usr/src/myapp/3rdparty
WORKDIR /usr/src/myapp

RUN 3rdparty/mcconf/install-python-libs.sh
RUN 3rdparty/install-libcxx.sh

COPY arch-config /usr/src/myapp/arch-config
COPY host-knc /usr/src/myapp/host-knc
COPY kernel /usr/src/myapp/kernel
COPY kernel-amd64.config /usr/src/myapp/kernel-amd64.config
COPY Makefile.user /usr/src/myapp/Makefile.user
RUN 3rdparty/mcconf/mcconf -i kernel-amd64.config

WORKDIR /usr/src/myapp/kernel-amd64
RUN make

CMD ["make", "qemu"]

LABEL Name=mythos Version=0.0.1
