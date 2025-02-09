FROM ubuntu:latest

RUN apt-get update && apt-get install -y \
    build-essential \
    git \
    g++ \
    cmake \
    wget \
    curl \
    libz-dev \
    python3 \
    python3-pip \
    && apt-get clean

RUN git clone https://boringssl.googlesource.com/boringssl && \
    cd boringssl && \
    mkdir build && cd build && \
    cmake .. && \
    make

RUN pip3 install dash