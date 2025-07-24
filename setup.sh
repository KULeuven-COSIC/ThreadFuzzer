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

function build_install_dir { 
    start_dir_l=`pwd`
    if ! build_dir $1; then
        return $?
    fi
    start_dir_l=`pwd`
    cd $1 && cd build && sudo make install;
    success=$?
    cd $pwd
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


function apt_install_all_packets {
    sudo apt update && sudo apt upgrade
    
    # GIT
    sudo apt install git -y

    # Clang
    wget https://apt.llvm.org/llvm.sh && chmod +x llvm.sh && sudo ./llvm.sh 18

    ## For WDissector
    sudo apt install -y libglib2.0-dev libc-ares-dev qtbase5-dev qtchooser qt5-qmake qtbase5-dev-tools qttools5-dev qtmultimedia5-dev libspeexdsp-dev libcap-dev libibverbs-dev

    ## For Fuzzer
    # LibSerial
    sudo apt install -y g++ git autogen autoconf build-essential cmake graphviz libboost-dev libboost-test-dev libgtest-dev libtool \
                python3-sip-dev doxygen python3-sphinx pkg-config python3-sphinx-rtd-theme android-tools-adb
    # ZMQ
    sudo apt-get install -y libzmq3-dev catch2
    # SPDLOG
    sudo apt install -y libspdlog-dev
    # Additional
    sudo apt install -y dbus-x11 python3 bear
}

function install_neccessary_libs() {
    # Apt install necessary packets
    apt_install_all_packets

    # Install everything in the third-party folder
    cd third-party
    start_dir_l=`pwd`

    # Install nlohmann json
    git clone https://github.com/nlohmann/json.git
    if ! build_install_dir json; then
        custom_exit $?
    fi
    cd $start_dir_l

    # Install libzmq
    git clone https://github.com/zeromq/libzmq.git && cd libzmq
    ./autogen.sh
    ./configure
    make
    sudo make install && sudo ldconfig
    cd $start_dir_l

    # Install cppzmq`
    git clone https://github.com/zeromq/cppzmq.git
    if ! build_install_dir cppzmq; then
        custom_exit $?
    fi
    cd $start_dir_l
    cd ..
}

### FUNCTION DEFINITIONS END

#################################################################################################################
# Main script execution starts here
cd "$script_dir" || exit_func 1

# Install all apt packets
install_neccessary_libs

# Remove randomization. That is due to the bug in ASAN, which causes crashes otherwise. 
# See https://stackoverflow.com/questions/78293129/c-programs-fail-with-asan-addresssanitizerdeadlysignal for details.
echo 0 | sudo tee /proc/sys/kernel/randomize_va_space

# Make sure that the submodules are updated
git submodule update --init --recursive || exit_func 1

## Build the shared memory library
echo "Building SHM"
fuzz_config_file_path=`pwd`/common/shm/config.json
sed -i "s|FUZZ_CONFIG_PATH_PLACEHOLDER|$fuzz_config_file_path|g" common/shm/fuzz_config.h
build_dir $SHM_PATH || exit_func 1

## Build the coverage instrumentation library
echo "Building Coverage instrumentation library"
build_dir $COV_INSTR_PATH || exit_func 1

## Build the ZMQ helper libraries
echo "Building ZMQ helper libraries"
build_dir $ZMQ_SERVER_PATH || exit_func 1
build_dir $ZMQ_CLIENT_PATH || exit_func 1

echo "Building OpenThread and BadThread"
## Checkout the OpenThread repository, apply patches and build it
checkout_tag "$OPENTHREAD_PATH" "$OPENTHREAD_CHECKOUT_TAG"
checkout_tag "$BADTHREAD_PATH" "$OPENTHREAD_CHECKOUT_TAG"

# # TODO: ADD CHECK IF THE PATCHES ARE ALREADY APPLIED!
cd "$OPENTHREAD_PATH" && git apply --ignore-whitespace ../patches/openthread.patch && CFLAGS="${CFLAGS} -g -fsanitize=address -fsanitize-coverage=edge,no-prune,trace-pc-guard" CXXFLAGS="${CXXFLAGS} -g -fsanitize=address -fsanitize-coverage=edge,no-prune,trace-pc-guard"  CXX=/usr/bin/clang++-18 CC=/usr/bin/clang-18 bear -- ./script/cmake-build simulation -DOT_FULL_LOGS=ON -DOT_THREAD_VERSION=1.3 && cd - || exit_func 1
cd "$BADTHREAD_PATH" && git apply --ignore-whitespace ../patches/badthread.patch && CFLAGS="${CFLAGS} -g -fsanitize=address -fsanitize-coverage=edge,no-prune,trace-pc-guard" CXXFLAGS="${CXXFLAGS} -g -fsanitize=address -fsanitize-coverage=edge,no-prune,trace-pc-guard" CXX=/usr/bin/clang++-18 CC=/usr/bin/clang-18 bear -- ./script/cmake-build simulation -DOT_FULL_LOGS=ON -DOT_THREAD_VERSION=1.3 && cd - || exit_func 1

## Step 3: Build the WDissector
# Install the dependencies
echo "Building WDissector"
build_dir "$CUSTOM_WIRESHARK_PATH" || exit_func 1
build_dir "$WDISSECTOR_PATH" || exit_func 1

## Step 4: Build the Fuzzer
build_dir "." || exit_func 1

# Exit successfully
exit_func 0