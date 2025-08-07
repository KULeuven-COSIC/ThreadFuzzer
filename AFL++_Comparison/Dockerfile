FROM ubuntu:22.04

WORKDIR /app/OpenThreadAFLFuzzing

ARG BASE_DIR=/app/OpenThreadAFLFuzzing
ENV BASE_DIR=${BASE_DIR}

ENV DEBIAN_FRONTEND=noninteractive

RUN apt update
RUN apt-get install -y build-essential python3-dev automake cmake git flex bison libglib2.0-dev libpixman-1-dev python3-setuptools cargo libgtk-3-dev
RUN apt install -y wget lsb-release software-properties-common gnupg

# Install clang
RUN wget https://apt.llvm.org/llvm.sh && chmod +x llvm.sh && ./llvm.sh 18

# Install AFL++
RUN git clone https://github.com/AFLplusplus/AFLplusplus
RUN cd AFLplusplus && make all && make install

# Install GIT
RUN apt install git -y

RUN git clone https://github.com/openthread/openthread.git

ARG OT_CHECKOUT_TAG="thread-reference-20230706"
RUN git -C "openthread" checkout "$OT_CHECKOUT_TAG"

COPY openthread.patch ./openthread.patch
RUN cd "openthread" && git apply --ignore-whitespace ../openthread.patch

COPY afl_fuzz_compile.sh ./afl_fuzz_compile.sh
RUN apt install -y ninja-build
RUN ./afl_fuzz_compile.sh afl-clang-lto

COPY run_afl_fuzzing.sh ./run_afl_fuzzing.sh
COPY AFL++_Crash_Seeds/ ./AFL++_Crash_Seeds/
COPY dedup_afl_crashes.py ./dedup_afl_crashes.py