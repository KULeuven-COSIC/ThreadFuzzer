FROM ubuntu:22.04

WORKDIR /app/ThreadFuzzer
ARG BASE_DIR=/app/ThreadFuzzer
ENV BASE_DIR=${BASE_DIR}

ENV DEBIAN_FRONTEND=noninteractive

RUN apt update && apt upgrade
# Install GIT
RUN apt install git -y

# Clang
RUN apt install -y wget lsb-release software-properties-common gnupg
RUN wget https://apt.llvm.org/llvm.sh && chmod +x llvm.sh && ./llvm.sh 18

## For WDissector
RUN apt install -y libglib2.0-dev libc-ares-dev qtbase5-dev qtchooser qt5-qmake qtbase5-dev-tools qttools5-dev qtmultimedia5-dev libspeexdsp-dev libcap-dev libibverbs-dev

## For Fuzzer
# LibSerial
RUN apt install -y g++ git autogen autoconf build-essential cmake graphviz libboost-dev libboost-test-dev libgtest-dev libtool \
             python3-sip-dev doxygen python3-sphinx pkg-config python3-sphinx-rtd-theme
# ZMQ
RUN apt-get install -y libzmq3-dev catch2
# SPDLOG
RUN apt install -y libspdlog-dev
# Additional
RUN apt install -y dbus-x11 python3 udo psmisc nano procps libcanberra-gtk-module libcanberra-gtk3-module bear ninja-build

# Install spdlog
RUN git clone https://github.com/crayzeewulf/libserial.git && cd libserial && ./compile.sh && cd build && make install

COPY third-party/ ./third-party/

# Build wireshark
RUN apt install -y libgcrypt-dev flex libpcap-dev
RUN cd third-party/wdissector/libs/wireshark && rm -rf build && mkdir -p build && cd build && cmake .. && make

# Build WDissector
RUN cd third-party/wdissector/ && rm -rf build && mkdir -p build && cd build && cmake .. && make

# Set CMake compiler
ENV CC=/usr/bin/clang-18
ENV CXX=/usr/bin/clang++-18

## Build common dir
COPY common/Coverage_Instrumentation/ ./common/Coverage_Instrumentation/
COPY common/ZMQ/ ./common/ZMQ/
COPY common/shm/*.cpp ./common/shm/
COPY common/shm/*.h ./common/shm/
COPY common/shm/*.txt ./common/shm/
# Set the correct path to shm config file
ARG fuzz_config_file_path=/app/ThreadFuzzer/common/shm/config.json
RUN sed -i "s|FUZZ_CONFIG_PATH_PLACEHOLDER|$fuzz_config_file_path|g" common/shm/fuzz_config.h

# Install libzmq
RUN git clone https://github.com/zeromq/libzmq.git && cd libzmq && ./autogen.sh && ./configure && make && make install && ldconfig

# Install cppzmq
RUN git clone https://github.com/zeromq/cppzmq.git && cd cppzmq && mkdir -p build && cd build && cmake -DCPPZMQ_BUILD_TESTS=OFF .. && make && make install

# Build common dir
RUN cd common/Coverage_Instrumentation/ && rm -rf build && mkdir -p build && cd build && cmake .. && make
RUN cd common/ZMQ/ZMQ_Client/ && rm -rf build && mkdir -p build && cd build && cmake .. && make
RUN cd common/ZMQ/ZMQ_Server/ && rm -rf build && mkdir -p build && cd build && cmake .. && make

# Install NLOHMANN JSON
RUN git clone https://github.com/nlohmann/json.git && cd json && mkdir -p build && cd build && cmake .. && make && make install
RUN cd common/shm/ && rm -rf build && mkdir -p build && cd build && cmake .. && make

# Checkout, apply patches and build openthread and badthread
ARG OT_CHECKOUT_TAG="thread-reference-20230706"
RUN git -C "third-party/openthread" checkout "$OT_CHECKOUT_TAG"
RUN git -C "third-party/badthread" checkout "$OT_CHECKOUT_TAG"
RUN cd third-party/openthread && git apply --ignore-whitespace ../patches/openthread.patch && CFLAGS="${CFLAGS} -g -fsanitize=address -fsanitize-coverage=edge,no-prune,trace-pc-guard" CXXFLAGS="${CXXFLAGS} -g -fsanitize=address -fsanitize-coverage=edge,no-prune,trace-pc-guard"  CXX=/usr/bin/clang++-18 CC=/usr/bin/clang-18 bear -- ./script/cmake-build simulation -DOT_FULL_LOGS=ON -DOT_THREAD_VERSION=1.3
RUN cd third-party/badthread && git apply --ignore-whitespace ../patches/badthread.patch && CFLAGS="${CFLAGS} -g -fsanitize=address -fsanitize-coverage=edge,no-prune,trace-pc-guard" CXXFLAGS="${CXXFLAGS} -g -fsanitize=address -fsanitize-coverage=edge,no-prune,trace-pc-guard"  CXX=/usr/bin/clang++-18 CC=/usr/bin/clang-18 bear -- ./script/cmake-build simulation -DOT_FULL_LOGS=ON -DOT_THREAD_VERSION=1.3

# Build ThreadFuzzer
COPY src/ ./src/
COPY include/ ./include/
COPY CMakeLists.txt ./

RUN rm -rf build && mkdir -p build && cd build && cmake .. && make

# Unset compiler-specifying variables
RUN unset CC
RUN unset CXX

## Preparation for the runtime
COPY common/ ./common/
COPY configs/ ./configs/
COPY seeds/ ./seeds/

ENTRYPOINT ["/bin/bash", "-l", "-c"]
# CMD [ "./build/ThreadFuzzer" ]