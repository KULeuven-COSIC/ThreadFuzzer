#!/bin/bash

# Helper function to display script usage
usage() {
    echo "Usage: $0 <number_of_iterations>"
    echo "Builds the thread_fuzzer docker image and runs it for the specified number of iterations."
    echo ""
    echo "Arguments:"
    echo "  <number_of_iterations>  A positive integer indicating how many times to run the loop."
    echo "  -h, --help              Show this help message and exit."
    echo ""
    echo "Example:"
    echo "  $0 5"
    exit 1
}

# Check for help flags
if [[ "$1" == "-h" || "$1" == "--help" ]]; then
    usage
fi

# Check if the parameter is provided and is a valid positive integer
if [[ -z "$1" ]] || ! [[ "$1" =~ ^[0-9]+$ ]]; then
    echo "Error: Missing or invalid number of iterations."
    echo ""
    usage
fi

ITERATIONS=$1

# Build the docker image
sudo docker build --build-arg BUILD_CORES=3 --progress=plain -t thread_fuzzer:latest .

# Run the fuzzer loop
# Using 'seq -w' keeps the zero-padded formatting (e.g., 01, 02) depending on the max number
for i in $(seq -w 1 "$ITERATIONS"); do
    echo "Starting iteration: $i of $ITERATIONS"
    
    sudo docker run --security-opt apparmor=unconfined \
                -v build:/app/ThreadFuzzer/build \
                -v /var/run/dbus:/var/run/dbus \
                -v otbr-log:/app/ThreadFuzzer/otbr-log \
                -v logs:/app/ThreadFuzzer/logs \
                --device=/dev/net/tun \
                --device=/dev/ttyACM0 \
                --device=/dev/ttyUSB0 \
                --cap-add=NET_ADMIN \
                --cap-add=SYS_PTRACE \
                --network=host \
                --name=threadfuzzer --rm -it thread_fuzzer
                
    echo "Iteration $i completed. Sleeping for 10 seconds..."
    sleep 10
done

echo "All $ITERATIONS iterations completed."