#!/bin/bash

# Fail if no argument is provided
if [ -z "$1" ]; then
    echo "Usage: $0 <fuzzer>"
    echo "Supported fuzzers: universal-mle-fuzzer-1, universal-mle-fuzzer-2"
    exit 1
fi

FUZZER="$1"

case "$FUZZER" in
    universal-mle-fuzzer-1)
        ;;
    universal-mle-fuzzer-2)
        ;;
    *)
        echo "Unsupported fuzzer: $FUZZER"
        echo "Supported fuzzers: universal-mle-fuzzer-1, universal-mle-fuzzer-2"
        exit 1
        ;;
esac

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

# Run afl with the constraint for the packet length: it should be between 6 and 174 bytes long to make sure it is not rejected by DUT.
afl-fuzz \
    -g 6 \
    -G 174 \
    -o openthread/tests/improved_fuzz/${FUZZER}_out/ \
    -i openthread/tests/improved_fuzz/corpora/${FUZZER}/ \
    -- ${SCRIPT_DIR}/openthread/build/simulation/tests/improved_fuzz/${FUZZER} @@
