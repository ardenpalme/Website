FROM ubuntu:latest

RUN apt-get update && apt-get install -y \
    gcc \
    g++ \
    cmake \
    libjsoncpp-dev \
    uuid-dev \
    zlib1g-dev \
    openssl \
    libssl-dev \
    git \
    libyaml-cpp-dev \
    && apt-get clean

RUN cd /opt \
    && git clone https://github.com/drogonframework/drogon \
    && cd drogon \
    && git submodule update --init \
    && mkdir build \
    && cd build \
    && cmake .. \
    && make \
    && make install