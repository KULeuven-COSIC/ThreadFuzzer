FROM ubuntu:22.04

WORKDIR /app/ThreadFuzzer
ARG BASE_DIR=/app/ThreadFuzzer
ENV BASE_DIR=${BASE_DIR}
ENV DEBIAN_FRONTEND=noninteractive

# TODO: Pin the exact versions used
RUN apt-get update && apt-get -y install --no-install-recommends \
    apt-utils \
    autoconf \
    autogen \
    automake \
    bear \
    build-essential \
    ca-certificates \
    catch2 \
    clang \
    cmake \
    curl \
    dbus-x11 \
    doxygen \
    flex \
    g++ \
    git \
    graphviz \
    libavahi-client-dev \
    libavahi-common-dev \
    libboost-dev \
    libboost-filesystem-dev \
    libboost-system-dev \
    libboost-test-dev \
    libc-ares-dev \
    libcanberra-gtk-module \
    libcanberra-gtk3-module \
    libcap-dev \
    libcpputest-dev \
    libevent-dev \
    libgcrypt-dev \
    libglib2.0-dev \
    libgtest-dev \
    libibverbs-dev \
    libncurses-dev \
    libnetfilter-queue-dev \
    libpcap-dev \
    libreadline-dev \
    libspdlog-dev \
    libspeexdsp-dev \
    libssl-dev \
    libtool \
    libzmq3-dev \
    lsb-release \
    nano \
    ninja-build \
    openssl \
    pip \
    pkg-config \
    procps \
    psmisc \
    python3 \
    python3-sip-dev \
    python3-sphinx \
    python3-sphinx-rtd-theme \
    python3.11 \
    python3.11-dev \
    python3.11-venv \
    qt5-qmake \
    qtbase5-dev \
    qtbase5-dev-tools \
    qtchooser \
    qtmultimedia5-dev \
    qttools5-dev \
    sudo \
    virtualenv \
    wget \
    && apt-get clean && rm -rf /var/lib/apt/lists/*

# Use a single core to build by default
ARG BUILD_CORES=1
RUN echo "==================================================" \
 && echo " NOTICE: You are using ${BUILD_CORES} cores!" \
 && echo "=================================================="

# first install the border router (it does not like systemctl)
ARG INFRA_IF_NAME
ARG BORDER_ROUTING
ARG BACKBONE_ROUTER
ARG OT_BACKBONE_CI
ARG OTBR_OPTIONS
ARG DNS64
ARG NAT64
ARG NAT64_SERVICE
ARG NAT64_DYNAMIC_POOL
ARG REFERENCE_DEVICE
ARG RELEASE
ARG REST_API
ARG WEB_GUI
ARG MDNS

ENV INFRA_IF_NAME=${INFRA_IF_NAME:-eth0}
ENV BORDER_ROUTING=${BORDER_ROUTING:-1}
ENV BACKBONE_ROUTER=${BACKBONE_ROUTER:-1}
ENV OT_BACKBONE_CI=${OT_BACKBONE_CI:-0}
ENV OTBR_MDNS=${MDNS:-mDNSResponder}
ENV OTBR_OPTIONS=${OTBR_OPTIONS}
ENV DEBIAN_FRONTEND=noninteractive
ENV PLATFORM=ubuntu
ENV REFERENCE_DEVICE=${REFERENCE_DEVICE:-0}
ENV RELEASE=${RELEASE:-1}
ENV NAT64=${NAT64:-1}
ENV NAT64_SERVICE=${NAT64_SERVICE:-openthread}
ENV NAT64_DYNAMIC_POOL=${NAT64_DYNAMIC_POOL:-192.168.255.0/24}
ENV DNS64=${DNS64:-0}
ENV WEB_GUI=${WEB_GUI:-1}
ENV REST_API=${REST_API:-1}
ENV DOCKER=1
ENV CC=clang
ENV CXX=clang++

### Install third-party tools
## Install spdlog
RUN git clone https://github.com/crayzeewulf/libserial.git
WORKDIR /app/ThreadFuzzer/libserial
RUN ./compile.sh
WORKDIR /app/ThreadFuzzer/libserial/build
RUN make -j${BUILD_CORES} install
WORKDIR /app/ThreadFuzzer

## Install NLOHMANN JSON
RUN git clone https://github.com/nlohmann/json.git
WORKDIR /app/ThreadFuzzer/json
RUN git checkout tags/v3.12.0 && mkdir -p build
WORKDIR /app/ThreadFuzzer/json/build
RUN cmake .. && make -j${BUILD_CORES} && make install && ldconfig
WORKDIR /app/ThreadFuzzer

## Install libzmq
RUN git clone https://github.com/zeromq/libzmq.git
WORKDIR /app/ThreadFuzzer/libzmq
RUN ./autogen.sh && ./configure && make -j${BUILD_CORES} && make install && ldconfig
WORKDIR /app/ThreadFuzzer

## Install cppzmq
RUN git clone https://github.com/zeromq/cppzmq.git
WORKDIR /app/ThreadFuzzer/cppzmq
RUN mkdir -p build
WORKDIR /app/ThreadFuzzer/cppzmq/build
RUN cmake -DCPPZMQ_BUILD_TESTS=OFF .. && make -j${BUILD_CORES} && make install
WORKDIR /app/ThreadFuzzer

### Copy and build the common directory
COPY common/Coverage_Instrumentation/ ./common/Coverage_Instrumentation/
COPY common/ZMQ/ ./common/ZMQ/
COPY common/shm/*.cpp ./common/shm/
COPY common/shm/*.h ./common/shm/
COPY common/shm/*.txt ./common/shm/

## Set the correct path to shm config file
ARG fuzz_config_file_path=/app/ThreadFuzzer/common/shm/config.json
RUN sed -i "s|FUZZ_CONFIG_PATH_PLACEHOLDER|$fuzz_config_file_path|g" common/shm/fuzz_config.h

## Build the common dir
WORKDIR /app/ThreadFuzzer/common/shm
RUN rm -rf build && mkdir -p build
WORKDIR /app/ThreadFuzzer/common/shm/build
RUN cmake .. && make -j${BUILD_CORES}

WORKDIR /app/ThreadFuzzer/common/Coverage_Instrumentation
RUN rm -rf build && mkdir -p build
WORKDIR /app/ThreadFuzzer/common/Coverage_Instrumentation/build
RUN cmake .. && make -j${BUILD_CORES}

WORKDIR /app/ThreadFuzzer/common/ZMQ/ZMQ_Client
RUN rm -rf build && mkdir -p build
WORKDIR /app/ThreadFuzzer/common/ZMQ/ZMQ_Client/build
RUN cmake .. && make -j${BUILD_CORES}

WORKDIR /app/ThreadFuzzer/common/ZMQ/ZMQ_Server
RUN rm -rf build && mkdir -p build
WORKDIR /app/ThreadFuzzer/common/ZMQ/ZMQ_Server/build
RUN cmake .. && make -j${BUILD_CORES}

WORKDIR /app/ThreadFuzzer

### Copy and build the third-party directory
COPY third-party/ ./third-party/

## OTBR-POSIX
WORKDIR /app/ThreadFuzzer/third-party
RUN git clone https://github.com/openthread/ot-br-posix.git
WORKDIR /app/ThreadFuzzer/third-party/ot-br-posix
RUN git checkout "thread-reference-20230710" && git submodule update --init && git apply --ignore-whitespace ../patches/new_otbr.patch
WORKDIR /app/ThreadFuzzer/third-party/ot-br-posix/third_party/openthread/repo
RUN git apply --ignore-whitespace ../../../../patches/new_ot_in_otbr.patch
WORKDIR /app/ThreadFuzzer/third-party/ot-br-posix
RUN ./script/bootstrap && \
    CFLAGS="${CFLAGS} -g -fsanitize=address -fsanitize-coverage=edge,no-prune,trace-pc-guard" \
    CXXFLAGS="${CXXFLAGS} -g -fsanitize=address -fsanitize-coverage=edge,no-prune,trace-pc-guard" \
    INFRA_IF_NAME=${wlan_interface_name} \
    CXX=/usr/bin/clang++ CC=/usr/bin/clang \
    ./script/setup

WORKDIR /app/ThreadFuzzer
COPY syslog.conf ./syslog.conf
RUN mkdir -p var/spool/rsyslog otbr-log && touch otbr-log/syslog
# Make sure that rsyslog is installed first!! Assumes syslog exists as a user!
RUN chown -R syslog:adm /app/ThreadFuzzer/syslog.conf /app/ThreadFuzzer/var/ /app/ThreadFuzzer/otbr-log/
# make sure that avahi does not try to grab the dbus
RUN sed -i 's|#enable-dbus=yes|enable-dbus=no|g'  /etc/avahi/avahi-daemon.conf

## Wireshark
WORKDIR /app/ThreadFuzzer/third-party/wdissector/libs/wireshark
RUN rm -rf build && mkdir -p build
WORKDIR /app/ThreadFuzzer/third-party/wdissector/libs/wireshark/build
RUN cmake .. && make -j${BUILD_CORES}

## WDissector
WORKDIR /app/ThreadFuzzer/third-party/wdissector
RUN rm -rf build && mkdir -p build
WORKDIR /app/ThreadFuzzer/third-party/wdissector/build
RUN cmake .. && make -j${BUILD_CORES}

## OpenThread
WORKDIR /app/ThreadFuzzer/third-party
RUN git clone https://github.com/openthread/openthread.git \
    && git clone https://github.com/openthread/openthread.git badthread
ARG OT_CHECKOUT_TAG="thread-reference-20230706"
RUN git -C "openthread" checkout "$OT_CHECKOUT_TAG" && \
    git -C "badthread" checkout "$OT_CHECKOUT_TAG"
WORKDIR /app/ThreadFuzzer/third-party/openthread
RUN git apply --ignore-whitespace ../patches/openthread.patch &&\
    CFLAGS="${CFLAGS} -g -fsanitize=address -fsanitize-coverage=edge,no-prune,trace-pc-guard" \
    CXXFLAGS="${CXXFLAGS} -g -fsanitize=address -fsanitize-coverage=edge,no-prune,trace-pc-guard" \
    bear -- ./script/cmake-build simulation -DOT_FULL_LOGS=ON -DOT_THREAD_VERSION=1.3
WORKDIR /app/ThreadFuzzer/third-party/badthread
RUN git apply --ignore-whitespace ../patches/badthread.patch && \
    CFLAGS="${CFLAGS} -g -fsanitize=address -fsanitize-coverage=edge,no-prune,trace-pc-guard" \
    CXXFLAGS="${CXXFLAGS} -g -fsanitize=address -fsanitize-coverage=edge,no-prune,trace-pc-guard" \
    bear -- ./script/cmake-build simulation -DOT_FULL_LOGS=ON -DOT_THREAD_VERSION=1.3
WORKDIR /app/ThreadFuzzer

# Create an environment
RUN virtualenv --python="/usr/bin/python3.11" venv/

# THE BIG PROBLEM: WE RUN EVERYTHING ON UBUNTU 22.04 -> THEREFORE WE DON'T HAVE PYTHON>=3.11!!IN THE TYPICAL PYTHON PATH!
## ConnectedHomeIP
RUN git clone --depth=1 https://github.com/project-chip/connectedhomeip.git
WORKDIR /app/ThreadFuzzer/connectedhomeip
RUN ./scripts/checkout_submodules.py --shallow --recursive --platform linux
RUN . ../venv/bin/activate && bash scripts/bootstrap.sh && mkdir out
RUN . ../venv/bin/activate && bash scripts/activate.sh && bash scripts/examples/gn_build_example.sh examples/chip-tool out/
WORKDIR /app/ThreadFuzzer

################################################################ CLEAN ################################################################

# apply patch to the server script for the mdns and webgui:
COPY fix_mdns_webgui.patch ./
WORKDIR /app/ThreadFuzzer/third-party/ot-br-posix
RUN git apply --ignore-whitespace ../../fix_mdns_webgui.patch
WORKDIR /app/ThreadFuzzer

### ThreadFuzzer
COPY src/ ./src/
COPY include/ ./include/
COPY CMakeLists.txt ./
RUN mkdir -p build

## Preparation for the runtime
COPY common/shm/config.json ./common/shm/config.json
COPY configs/ ./configs/
COPY seeds/ ./seeds/
COPY scripts/ ./scripts/
COPY bin/ ./bin/
COPY coverage_log/ ./coverage_log/
COPY run_experiment.sh ./
RUN chmod +x scripts/*.sh run_experiment.sh

ENTRYPOINT ["scripts/docker_entrypoint.sh"]
