#!/bin/bash

### CONSTANT DEFINITIONS START
readonly OPENTHREAD_PATH="third-party/openthread"
readonly BADTHREAD_PATH="third-party/badthread"
readonly OPENTHREAD_CHECKOUT_TAG="thread-reference-20230706"
readonly OT_BR_POSIX_PATH="third-party/ot-br-posix"
readonly OT_BR_POSIX_CHECKOUT_TAG="thread-reference-20230710"
readonly WDISSECTOR_PATH="third-party/wdissector"
readonly CUSTOM_WIRESHARK_PATH="third-party/wdissector/libs/wireshark"

readonly SHM_PATH="common/shm"
readonly COV_INSTR_PATH="common/Coverage_Instrumentation"
readonly ZMQ_SERVER_PATH="common/ZMQ/ZMQ_Client"
readonly ZMQ_CLIENT_PATH="common/ZMQ/ZMQ_Server"

### CONSTANT DEFINITIONS END

### VARIABLE DEFINITIONS START
init_path="$(pwd)"
script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
all_interface_names_arr=($(ip -4 -brief address show | awk '{print $1}' | tr -d ' '))
all_interface_names_str=$(printf "%s " "${all_interface_names_arr[@]}")
default_wlan_interface_name="$(ip -o -4 route show to default | awk '{print $5}')"
### VARIABLE DEFINITIONS END

### FUNCTION DEFINITIONS START
function build_success { echo "Build successful"; }
function build_fail { echo "Build failed"; }
# function build_routine { mkdir -p build && cd build && CFLAGS="${CFLAGS} -g -fsanitize=address" CXXFLAGS="${CXXFLAGS} -g -fsanitize=address" CC=/usr/bin/clang-18 CXX=/usr/bin/clang++-18 cmake .. && bear -- make -j15; }
function build_routine { mkdir -p build && cd build && CC=/usr/bin/clang-18 CXX=/usr/bin/clang++-18 cmake .. && cmake --build . -j3; }
function build_dir { 
    success=0;
    start_dir=`pwd`;
    echo "Building $1";
    if cd $1 && build_routine; then
        build_success;
    else
        success=$?;
        build_fail;
    fi
    cd $start_dir;
    return $success
}

# Function to reset to the initial directory and exit with a given status code
function exit_func() {
    cd "$init_path"
    exit "$1"
}

# Function to checkout a specific Git tag in a given repository path
function checkout_tag() {
    local repo_path="$1"
    local tag="$2"

    if [[ -d "$repo_path" ]]; then
        echo "Checking out tag $tag in repository $repo_path..."
        git -C "$repo_path" checkout "$tag" || {
            echo "Error: Failed to checkout $tag in $repo_path"
            exit_func 1
        }
    else
        echo "Error: Repository path $repo_path does not exist."
        exit_func 1
    fi
}

# Helper function to determine if the value is in array. Returns 0 if the value is in the array.
function is_value_in_array() {
    local val=$1
    shift
    for element in "$@"; do
        if [[ "$element" == "$val" ]]; then
            return 0  # Return 0 (success) if the value is found
        fi
    done
    return 1  # Return 1 (failure) if the value is not found
}

function get_wlan_interface_name() {
    # Prompt the user to confirm if the default value is okay
    read -p "OT-BR-POSIX requires the internet access to allow Border routing. Can we use \"$default_wlan_interface_name\" interface? (y/n): " use_default
    # Check the user's response
    if [[ "$use_default" =~ ^[Yy]$ ]]; then
        # User agreed, so use the default value
        wlan_interface_name="$default_wlan_interface_name"
    else
        while true; do
            # User did not agree, prompt them to enter a custom string
            read -p "Please enter the name of the interface to be used (choose from $all_interface_names_str): " wlan_interface_name
            if is_value_in_array $wlan_interface_name ${all_interface_names_arr[@]}; then
                break
            else
                echo "${wlan_interface_name} is not in the list. Please try again."
            fi
        done
    fi
    echo "Using ${wlan_interface_name} interface"
    return 0
}

### FUNCTION DEFINITIONS END

#################################################################################################################
# Main script execution starts here
cd "$script_dir" || exit_func 1

# Remove randomization. That is due to the bug in ASAN, which causes crashes otherwise. 
# See https://stackoverflow.com/questions/78293129/c-programs-fail-with-asan-addresssanitizerdeadlysignal for details.
echo 0 | sudo tee /proc/sys/kernel/randomize_va_space

# Make sure that the submodules are updated
git submodule update --init --recursive || exit_func 1

## Step X: Build the shared memory library
build_dir $SHM_PATH || exit_func 1

## Step X: Build the coverage instrumentation library
build_dir $COV_INSTR_PATH || exit_func 1

build_dir $ZMQ_SERVER_PATH || exit_func 1
build_dir $ZMQ_CLIENT_PATH || exit_func 1

## Step 1: Checkout the OpenThread repository and build it
# checkout_tag "$OPENTHREAD_PATH" "$OPENTHREAD_CHECKOUT_TAG"
# checkout_tag "$BADTHREAD_PATH" "$OPENTHREAD_CHECKOUT_TAG"
# TODO: Let the user choose the mode to compile OpenThread in
# echo "\nBuilding OpenThread"
# cd "$OPENTHREAD_PATH" && ./script/bootstrap && cd -
# cd "$OPENTHREAD_PATH" && CFLAGS="${CFLAGS} -g -fsanitize=address" CXXFLAGS="${CXXFLAGS} -g -fsanitize=address" CXX=/usr/bin/clang++-18 CC=/usr/bin/clang-18 bear -- ./script/cmake-build simulation -DOT_THREAD_VERSION=1.3 && cd - || exit_func 1
cd "$OPENTHREAD_PATH" && CFLAGS="${CFLAGS} -g -fsanitize=address -fsanitize-coverage=edge,no-prune,trace-pc-guard" CXXFLAGS="${CXXFLAGS} -g -fsanitize=address -fsanitize-coverage=edge,no-prune,trace-pc-guard"  CXX=/usr/bin/clang++-18 CC=/usr/bin/clang-18 bear -- ./script/cmake-build simulation -DOT_FULL_LOGS=ON -DOT_THREAD_VERSION=1.3 && cd - || exit_func 1
cd "$BADTHREAD_PATH" && CFLAGS="${CFLAGS} -g -fsanitize=address -fsanitize-coverage=edge,no-prune,trace-pc-guard" CXXFLAGS="${CXXFLAGS} -g -fsanitize=address -fsanitize-coverage=edge,no-prune,trace-pc-guard" CXX=/usr/bin/clang++-18 CC=/usr/bin/clang-18 bear -- ./script/cmake-build simulation -DOT_FULL_LOGS=ON -DOT_THREAD_VERSION=1.3 && cd - || exit_func 1
# cd "$OPENTHREAD_PATH" && ./script/cmake-build simulation && ./script/cmake-build posix -DO_DAEMON=ON && cd - || exit_func 1


## Step 2: Checkout the ot-br-posix repository and build it
# checkout_tag "$OT_BR_POSIX_PATH" "$OT_BR_POSIX_CHECKOUT_TAG"
# get_wlan_interface_name || exit_func 1
# echo "\nBuilding OT-BR-POSIX"
# cd "$OT_BR_POSIX_PATH" && ./script/bootstrap && cd -
# # Needs those flags: -fsanitize=address -fsanitize-coverage=edge,no-prune,trace-pc-guard -Wno-unused-result -Wno-unused-but-set-variable -Wno-documentation -Wno-vla-cxx-extension
# cd "$OT_BR_POSIX_PATH" && CFLAGS="${CFLAGS} -g -fsanitize=address -fsanitize-coverage=edge,no-prune,trace-pc-guard" CXXFLAGS="${CXXFLAGS} -g -fsanitize=address -fsanitize-coverage=edge,no-prune,trace-pc-guard" INFRA_IF_NAME=${wlan_interface_name} CXX=/usr/bin/clang++-18 CC=/usr/bin/clang-18 ./script/setup && cd - || exit_func 1
# cd "$OT_BR_POSIX_PATH" && INFRA_IF_NAME=$wlan_interface_name ./script/setup && cd - || exit_func 1


## Step 3: Build the WDissector
# Install the dependencies
echo "\nBuilding WDissector\n"
# #sudo apt install -y libglib2.0-dev libc-ares-dev qtbase5-dev qtchooser qt5-qmake qtbase5-dev-tools qttools5-dev qtmultimedia5-dev libspeexdsp-dev libcap-dev libibverbs-dev
#build_dir "$CUSTOM_WIRESHARK_PATH" || exit_func 1
build_dir "$WDISSECTOR_PATH" || exit_func 1

## Step 4: Build the Fuzzer
build_dir "." || exit_func 1

# Exit successfully
exit_func 0
