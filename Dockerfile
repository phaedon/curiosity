# Use Ubuntu 20.04 as the base
FROM ubuntu:20.04

# Avoid interactive prompts
ENV DEBIAN_FRONTEND=noninteractive

# 1. Install system basics
RUN apt-get update && apt-get install -y \
    build-essential \
    curl \
    git \
    python3-pip \
    libstdc++-10-dev \
    lsb-release \
    gnupg \
    software-properties-common \
    && rm -rf /var/lib/apt/lists/*

# 2. Install CMake 3.25.2 via pip
RUN pip3 install cmake==3.25.2

# 3. Install Clang 18
RUN curl -LO https://apt.llvm.org/llvm.sh && \
    chmod +x llvm.sh && \
    ./llvm.sh 18 all && \
    rm llvm.sh

# Set Clang as the default compiler
ENV CC=clang-18
ENV CXX=clang++-18

# 4. Build Abseil (Static)
WORKDIR /tmp/abseil
RUN git clone https://github.com/abseil/abseil-cpp.git . && \
    git checkout 20240116.0 && \
    mkdir build && cd build && \
    cmake .. -DCMAKE_CXX_STANDARD=17 -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DABSL_ENABLE_INSTALL=ON -DBUILD_SHARED_LIBS=OFF -DCMAKE_INSTALL_PREFIX=/usr/local && \
    make -j$(nproc) install

# 5. Build Protobuf (Static)
WORKDIR /tmp/protobuf
RUN git clone https://github.com/protocolbuffers/protobuf.git . && \
    git checkout v27.2 && \
    git submodule update --init --recursive && \
    mkdir build && cd build && \
    cmake .. -Dprotobuf_BUILD_TESTS=OFF -Dprotobuf_BUILD_SHARED_LIBS=OFF -Dprotobuf_ABSL_PROVIDER=package -DCMAKE_PREFIX_PATH=/usr/local -DCMAKE_INSTALL_PREFIX=/usr/local && \
    make -j$(nproc) install && \
    ldconfig

# Clean up tmp files to keep the image small
WORKDIR /
RUN rm -rf /tmp/*

# This is your final working directory
WORKDIR /workspace