FROM ubuntu:22.04

WORKDIR /app/ThreadFuzzer
ARG BASE_DIR=/app/ThreadFuzzer
ENV BASE_DIR=${BASE_DIR}
ENV DEBIAN_FRONTEND=noninteractive


RUN apt update -y
# Install GIT
RUN apt install git -y

RUN apt install vim -y

## Build common dir
COPY common/Coverage_Instrumentation/ ./common/Coverage_Instrumentation/
COPY common/ZMQ/ ./common/ZMQ/
COPY common/shm/*.cpp ./common/shm/
COPY common/shm/*.h ./common/shm/
COPY common/shm/*.txt ./common/shm/


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

RUN apt install -y clang

RUN apt-get install --no-install-recommends -y sudo python3 lsb-release apt-utils build-essential psmisc ninja-build cmake wget ca-certificates libreadline-dev libncurses-dev libcpputest-dev libavahi-common-dev libavahi-client-dev libboost-dev libboost-filesystem-dev libboost-system-dev libnetfilter-queue-dev

# ## For Fuzzer
# LibSerial
RUN apt install --no-install-recommends -y g++ git autogen autoconf build-essential cmake graphviz libboost-dev libboost-test-dev libgtest-dev libtool \
             python3-sip-dev doxygen python3-sphinx pkg-config python3-sphinx-rtd-theme
# ZMQ
RUN apt-get install -y libzmq3-dev catch2
# SPDLOG
RUN apt install -y libspdlog-dev
# Additional
RUN apt install -y dbus-x11 python3 udo psmisc nano procps libcanberra-gtk-module libcanberra-gtk3-module bear ninja-build

# Install spdlog
RUN git clone https://github.com/crayzeewulf/libserial.git && cd libserial && ./compile.sh && cd build && make -j3 install

# Set the correct path to shm config file
ARG fuzz_config_file_path=/app/ThreadFuzzer/common/shm/config.json
RUN sed -i "s|FUZZ_CONFIG_PATH_PLACEHOLDER|$fuzz_config_file_path|g" common/shm/fuzz_config.h

# Install NLOHMANN JSON
RUN git clone https://github.com/nlohmann/json.git && cd json && git checkout tags/v3.12.0 && mkdir -p build && cd build && cmake .. && make -j3 && make install && ldconfig
RUN cd common/shm/ && rm -rf build && mkdir -p build && cd build && cmake .. && make -j3 

# Install libzmq
RUN apt install --no-install-recommends -y automake 
RUN git clone https://github.com/zeromq/libzmq.git && cd libzmq && ./autogen.sh && ./configure && make -j3 && make install && ldconfig

# Install cppzmq
RUN git clone https://github.com/zeromq/cppzmq.git && cd cppzmq && mkdir -p build && cd build && cmake -DCPPZMQ_BUILD_TESTS=OFF .. && make -j3 && make install

# Build common dir
RUN cd common/Coverage_Instrumentation/ && rm -rf build && mkdir -p build && cd build && cmake .. && make -j3 
RUN cd common/ZMQ/ZMQ_Client/ && rm -rf build && mkdir -p build && cd build && cmake .. && make -j3
RUN cd common/ZMQ/ZMQ_Server/ && rm -rf build && mkdir -p build && cd build && cmake .. && make -j3

COPY third-party/ ./third-party/

RUN cd third-party && git clone https://github.com/openthread/ot-br-posix.git && cd ot-br-posix && git checkout "thread-reference-20230710" && git submodule update --init

# RUN cd third-party/ot-br-posix && git apply --ignore-whitespace ../patches/0001-mdns.patch 
RUN cd third-party/ot-br-posix && git apply --ignore-whitespace ../patches/new_otbr.patch
RUN cd third-party/ot-br-posix/third_party/openthread/repo && git apply --ignore-whitespace ../../../../patches/new_ot_in_otbr.patch

RUN cd third-party/ot-br-posix/ && ./script/bootstrap

RUN cd third-party/ot-br-posix && CFLAGS="${CFLAGS} -g -fsanitize=address -fsanitize-coverage=edge,no-prune,trace-pc-guard" CXXFLAGS="${CXXFLAGS} -g -fsanitize=address -fsanitize-coverage=edge,no-prune,trace-pc-guard" INFRA_IF_NAME=${wlan_interface_name} CXX=/usr/bin/clang++ CC=/usr/bin/clang ./script/setup

COPY syslog.conf /app/ThreadFuzzer/syslog.conf
RUN mkdir -p var/spool/rsyslog
RUN mkdir -p otbr-log
RUN touch otbr-log/syslog
# Make sure that rsyslog is installed first!! Assumes syslog exists as a user!
RUN chown -R syslog:adm /app/ThreadFuzzer/syslog.conf /app/ThreadFuzzer/var/ /app/ThreadFuzzer/otbr-log/

# make sure that avahi does not try to grab the dbus
RUN sed -i 's|#enable-dbus=yes|enable-dbus=no|g'  /etc/avahi/avahi-daemon.conf

# Clang TODO: note that this breaks everything!!! If we want the software-properties-common too, then it installs the systemctl!
# RUN apt install -y wget lsb-release software-properties-common gnupg

# ## For WDissector
RUN apt install --no-install-recommends -y libglib2.0-dev libc-ares-dev qtbase5-dev qtchooser qt5-qmake qtbase5-dev-tools qttools5-dev qtmultimedia5-dev libspeexdsp-dev libcap-dev libibverbs-dev



# Build wireshark
RUN apt install -y libgcrypt-dev flex libpcap-dev
RUN cd third-party/wdissector/libs/wireshark && rm -rf build && mkdir -p build && cd build && cmake .. && make -j3 

# Build WDissector
RUN cd third-party/wdissector/ && rm -rf build && mkdir -p build && cd build && cmake .. && make -j3 

# RUN wget https://apt.llvm.org/llvm.sh && chmod +x llvm.sh && ./llvm.sh 18
# # Set CMake compiler
# # ENV CC=/usr/bin/clang-18
# # ENV CXX=/usr/bin/clang++-18
# RUN apt install -y clang
# ENV CC=clang
# ENV CXX=clang++
# 






# Checkout, apply patches and build openthread and badthread
RUN cd third-party && git clone https://github.com/openthread/openthread.git
RUN cd third-party && git clone https://github.com/openthread/openthread.git badthread
ARG OT_CHECKOUT_TAG="thread-reference-20230706"
RUN git -C "third-party/openthread" checkout "$OT_CHECKOUT_TAG"
RUN git -C "third-party/badthread" checkout "$OT_CHECKOUT_TAG"

RUN cd third-party/openthread && git apply --ignore-whitespace ../patches/openthread.patch && CFLAGS="${CFLAGS} -g -fsanitize=address -fsanitize-coverage=edge,no-prune,trace-pc-guard" CXXFLAGS="${CXXFLAGS} -g -fsanitize=address -fsanitize-coverage=edge,no-prune,trace-pc-guard" bear -- ./script/cmake-build simulation -DOT_FULL_LOGS=ON -DOT_THREAD_VERSION=1.3
# RUN cd third-party/openthread && git apply --ignore-whitespace ../patches/openthread.patch && bear -- ./script/cmake-build simulation -DOT_FULL_LOGS=ON -DOT_THREAD_VERSION=1.3
# RUN cd third-party/openthread && git apply --ignore-whitespace ../patches/openthread.patch && CFLAGS="${CFLAGS} -g -fsanitize=address " CXXFLAGS="${CXXFLAGS} -g -fsanitize=address " bear -- ./script/cmake-build simulation -DOT_FULL_LOGS=ON -DOT_THREAD_VERSION=1.3
RUN cd third-party/badthread && git apply --ignore-whitespace ../patches/badthread.patch && CFLAGS="${CFLAGS} -g -fsanitize=address -fsanitize-coverage=edge,no-prune,trace-pc-guard" CXXFLAGS="${CXXFLAGS} -g -fsanitize=address -fsanitize-coverage=edge,no-prune,trace-pc-guard" bear -- ./script/cmake-build simulation -DOT_FULL_LOGS=ON -DOT_THREAD_VERSION=1.3
# RUN cd third-party/badthread && git apply --ignore-whitespace ../patches/badthread.patch && bear -- ./script/cmake-build simulation -DOT_FULL_LOGS=ON -DOT_THREAD_VERSION=1.3
# RUN cd third-party/badthread && git apply --ignore-whitespace ../patches/badthread.patch && CFLAGS="${CFLAGS} -g -fsanitize=address" CXXFLAGS="${CXXFLAGS} -g -fsanitize=address " bear -- ./script/cmake-build simulation -DOT_FULL_LOGS=ON -DOT_THREAD_VERSION=1.3

# install yet another python version and create an environment
RUN apt install -y python3.11 python3.11-dev python3.11-venv virtualenv && virtualenv --python="/usr/bin/python3.11" venv/

# THE BIG PROBLEM: WE RUN EVERYTHING ON UBUNTU 22.04 -> THEREFORE WE DON'T HAVE PYTHON>=3.11!!IN THE TYPICAL PYTHON PATH!

RUN apt install -y pip python3.11-venv && git clone --depth=1 https://github.com/project-chip/connectedhomeip.git && cd connectedhomeip && ./scripts/checkout_submodules.py --shallow --recursive --platform  linux 

RUN apt install -y curl openssl libssl-dev sudo libevent-dev

RUN . venv/bin/activate && cd connectedhomeip && bash scripts/bootstrap.sh && mkdir out 

RUN . venv/bin/activate && cd connectedhomeip && bash scripts/activate.sh &&  bash scripts/examples/gn_build_example.sh examples/chip-tool out/

################################################################ CLEAN ################################################################

# apply patch to the server script for the mdns and webgui:
COPY fix_mdns_webgui.patch ./
RUN cd third-party/ot-br-posix && git apply --ignore-whitespace ../../fix_mdns_webgui.patch


# Build ThreadFuzzer
COPY src/ ./src/
COPY include/ ./include/
COPY CMakeLists.txt ./

# RUN rm -rf build && mkdir -p build && cd build && cmake .. && make 
RUN mkdir -p build


## Preparation for the runtime
COPY common/shm/config.json ./common/shm/config.json
COPY configs/ ./configs/
COPY seeds/ ./seeds/
COPY scripts/ ./scripts/
COPY bin/ ./bin/
COPY coverage_log/ ./coverage_log/

COPY run_experiment.sh ./

RUN chmod +x scripts/*.sh
RUN chmod +x run_experiment.sh
ENTRYPOINT ["scripts/docker_entrypoint.sh"]
