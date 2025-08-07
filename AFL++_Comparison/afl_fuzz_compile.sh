#!/bin/bash

# Fail if no argument is provided
if [ -z "$1" ]; then
    echo "Usage: $0 <compiler>"
    echo "Supported compilers: afl-clang-fast, afl-clang-lto"
    exit 1
fi

COMPILER="$1"

case "$COMPILER" in
    afl-clang-fast)
        ;;
    afl-clang-lto)
        ;;
    *)
        echo "Unsupported compiler: $COMPILER"
        echo "Supported compilers: afl-clang-fast, afl-clang-lto"
        exit 1
        ;;
esac

cd openthread
CXX=${COMPILER}++ CC=${COMPILER} AFL_USE_ASAN=1 \
	./script/cmake-build simulation \
	-DOT_BUILD_EXECUTABLES=OFF -DOT_FUZZ_TARGETS=ON -DOT_MTD=OFF -DOT_PLATFORM=external \
	-DOT_RCP=OFF -DOT_BORDER_AGENT=ON -DOT_BORDER_ROUTER=ON -DOT_CHANNEL_MANAGER=ON \
	-DOT_CHANNEL_MONITOR=ON -DOT_COAP=ON -DOT_COAPS=ON -DOT_COAP_BLOCK=ON -DOT_COAP_OBSERVE=ON \
	-DOT_COMMISSIONER=ON -DOT_DATASET_UPDATER=ON -DOT_DHCP6_CLIENT=ON -DOT_DHCP6_SERVER=ON \
	-DOT_DNS_CLIENT=ON -DOT_ECDSA=ON -DOT_HISTORY_TRACKER=ON -DOT_IP6_FRAGM=ON -DOT_JAM_DETECTION=ON \
	-DOT_JOINER=ON -DOT_LINK_RAW=ON -DOT_LOG_OUTPUT=APP -DOT_MAC_FILTER=ON -DOT_NETDATA_PUBLISHER=ON \
	-DOT_NETDIAG_CLIENT=ON -DOT_PING_SENDER=ON -DOT_SERVICE=ON -DOT_SLAAC=ON -DOT_SNTP_CLIENT=ON \
	-DOT_SRP_CLIENT=ON -DOT_SRP_SERVER=ON -DOT_THREAD_VERSION=1.3 -DOT_UPTIME=ON -DOT_DNS_DSO=OFF -DOT_COVERAGE=ON -DBUILD_TESTING=OFF
    
cd -