#!/usr/bin/env bash

set -eo pipefail

readonly STATE_FILE=".setup-script.state"
if [[ -f "$STATE_FILE" ]]; then
    last_done_step=$(<"$STATE_FILE")
    echo "Staring from step: $last_done_step"
else
    last_done_step=0
    echo "Starting from the beginning"
fi

### CONSTANT DEFINITIONS START
readonly THIRD_PARTY_PATH="third-party"
readonly OPENTHREAD_PATH="$THIRD_PARTY_PATH/openthread"
readonly BADTHREAD_PATH="$THIRD_PARTY_PATH/badthread"
readonly OPENTHREAD_CHECKOUT_TAG="thread-reference-20230706"
readonly OT_BR_POSIX_PATH="$THIRD_PARTY_PATH/ot-br-posix"
readonly OT_BR_POSIX_CHECKOUT_TAG="thread-reference-20230710"
readonly WDISSECTOR_PATH="$THIRD_PARTY_PATH/wdissector"
readonly CUSTOM_WIRESHARK_PATH="$THIRD_PARTY_PATH/wdissector/libs/wireshark"

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

# Function to checkout a specific Git tag in a given repository path
function checkout_tag() {
    local repo_path="$1"
    local tag="$2"

    # if [[ -d "$repo_path" ]]; then
    echo "Checking out tag $tag in repository $repo_path..."
    git -C "$repo_path" checkout "$tag"
    # else
    #     echo "Error: Repository path $repo_path does not exist."
    #     custom_exit 1
    # fi
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
    sudo apt install git 
    
    # Python
    sudo apt python3.5 python3-pip -y
    yes | pip3 install pandas matplotlib scipy

    ## For WDissector
    sudo apt install -y libglib2.0-dev libc-ares-dev qtbase5-dev \
        qtchooser qt5-qmake qtbase5-dev-tools qttools5-dev qtmultimedia5-dev \
        libspeexdsp-dev libcap-dev libibverbs-dev libgcrypt-dev flex libpcap-dev

    ## For Fuzzer
    # LibSerial
    sudo apt install -y g++ git autogen autoconf build-essential cmake graphviz libboost-dev libboost-test-dev libgtest-dev libtool \
                python3-sip-dev doxygen python3-sphinx pkg-config python3-sphinx-rtd-theme android-tools-adb
    # ZMQ
    sudo apt-get install -y libzmq3-dev
    # SPDLOG
    sudo apt install -y libspdlog-dev
    # Additional
    sudo apt install -y dbus-x11 python3 bear ninja-build
}

function save_step() {
    local step_num=$1
    echo "Saving step: $step_num"
    echo "$step_num" | sudo tee $STATE_FILE > /dev/null
}

function save_checkpoint() {
    save_step "$cur_step"
}

# Function to reset to the initial directory and exit with a given status code
function custom_exit() {
    local exit_code=$?
    cd "$init_path"
    exit $exit_code
}

function custom_err() {
    local error_code=$?
    echo "Error on step $cur_step; aborting (error code: $error_code)." >&2
    # last_successful_step=$(( cur_step - 1 ))
    # save_step "$last_successful_step"
}

### FUNCTION DEFINITIONS END

#################################################################################################################
# Initialize the variable responsible for keeping the state
cur_step=1

# Define custom handlers
trap custom_err ERR
trap custom_exit EXIT

# Remove randomization. That is due to the bug in ASAN, which causes crashes otherwise. 
# See https://stackoverflow.com/questions/78293129/c-programs-fail-with-asan-addresssanitizerdeadlysignal for details.
echo 0 | sudo tee /proc/sys/kernel/randomize_va_space

# Main script execution starts here
cd "$script_dir"

# Install all apt packets
if (( last_done_step < cur_step )); then
    echo "Step $cur_step: Installing apt packets"
    apt_install_all_packets
fi
save_checkpoint && (( cur_step++ ))


# Install Clang
if (( last_done_step < cur_step )); then
    echo "Step $cur_step: Installing clang"
    wget https://apt.llvm.org/llvm.sh && chmod +x llvm.sh && sudo ./llvm.sh 18
fi
save_checkpoint && (( cur_step++ ))

# Install nlohmann/json
cd "$script_dir"
cd $THIRD_PARTY_PATH
if (( last_done_step < cur_step )); then
    echo "Step $cur_step: Cloning nlohmann JSON"
    # Install nlohmann json
    git clone https://github.com/nlohmann/json.git
fi
save_checkpoint && (( cur_step++ ))
if (( last_done_step < cur_step )); then
    echo "Step $cur_step: Cloning nlohmann JSON"
    build_install_dir json
fi
save_checkpoint && (( cur_step++ ))

# Install libzmq
cd "$script_dir"
cd $THIRD_PARTY_PATH
if (( last_done_step < cur_step )); then
    echo "Step $cur_step: Cloning libzmq"
    git clone https://github.com/zeromq/libzmq.git
fi
save_checkpoint && (( cur_step++ ))
if (( last_done_step < cur_step )); then
    echo "Step $cur_step: Building libzmq"
    cd libzmq
    ./autogen.sh
    ./configure
    make
    sudo make install && sudo ldconfig
fi
save_checkpoint && (( cur_step++ ))

# Install cppzmq
cd "$script_dir"
cd $THIRD_PARTY_PATH
if (( last_done_step < cur_step )); then
    echo "Step $cur_step: Cloning cppzmq"
    git clone https://github.com/zeromq/cppzmq.git
fi
save_checkpoint && (( cur_step++ ))
if (( last_done_step < cur_step )); then
    echo "Step $cur_step: Building cppzmq"
    cd cppzmq
    mkdir -p build && cd build && CXX=/usr/bin/clang++-18 CC=/usr/bin/clang-18 cmake -DCPPZMQ_BUILD_TESTS=OFF .. && cmake --build . && sudo make install
fi
save_checkpoint && (( cur_step++ ))

## Build the shared memory library
cd "$script_dir"
if (( last_done_step < cur_step )); then
    echo "Step $cur_step: building SHM"
    fuzz_config_file_path=`pwd`/common/shm/config.json
    sed -i "s|FUZZ_CONFIG_PATH_PLACEHOLDER|$fuzz_config_file_path|g" common/shm/fuzz_config.h
    build_dir $SHM_PATH
fi
save_checkpoint && (( cur_step++ ))

## Build the coverage instrumentation library
cd "$script_dir"
if (( last_done_step < cur_step )); then
    echo "Step $cur_step: Coverage Instrumentation"
    build_dir $COV_INSTR_PATH
fi
save_checkpoint && (( cur_step++ ))

## Build the ZMQ Server
cd "$script_dir"
if (( last_done_step < cur_step )); then
    echo "Step $cur_step: Building ZMQ Server library"
    build_dir $ZMQ_SERVER_PATH
fi
save_checkpoint && (( cur_step++ ))

## Build the ZMQ Server
cd "$script_dir"
if (( last_done_step < cur_step )); then
    echo "Step $cur_step: Building ZMQ Client library"
    build_dir $ZMQ_CLIENT_PATH
fi
save_checkpoint && (( cur_step++ ))

# Make sure that the submodules are updated
# git submodule update --init --recursive
## For the anonymous Github: pull the necessary third-party libraries directrly from the source
cd "$script_dir"
if (( last_done_step < cur_step )); then
    echo "Step $cur_step: pulling the OpenThread"
    cd $THIRD_PARTY_PATH
    git clone https://github.com/openthread/openthread.git
fi
save_checkpoint && (( cur_step++ ))

cd "$script_dir"
if (( last_done_step < cur_step )); then
    echo "Step $cur_step: pulling the BadThread"
    cd $THIRD_PARTY_PATH
    git clone https://github.com/openthread/openthread.git badthread
fi
save_checkpoint && (( cur_step++ ))

cd "$script_dir"
if (( last_done_step < cur_step )); then
    echo "Step $cur_step: pulling the OT-BR"
    cd $THIRD_PARTY_PATH
    git clone https://github.com/openthread/ot-br-posix.git
fi
save_checkpoint && (( cur_step++ ))

cd "$script_dir"
## Checkout the OpenThread repository, apply patches and build it
checkout_tag "$OPENTHREAD_PATH" "$OPENTHREAD_CHECKOUT_TAG"
checkout_tag "$BADTHREAD_PATH" "$OPENTHREAD_CHECKOUT_TAG"
checkout_tag "$OT_BR_POSIX_PATH" "$OT_BR_POSIX_CHECKOUT_TAG"

## Apply patches and build OpenThread
cd "$OPENTHREAD_PATH"
if (( last_done_step < cur_step )); then
    echo "Step $cur_step: Applying patches to OpenThread"
    git apply --ignore-whitespace ../patches/openthread.patch
fi
save_checkpoint && (( cur_step++ ))
if (( last_done_step < cur_step )); then
    echo "Step $cur_step: Building OpenThread"
    CFLAGS="${CFLAGS} -g -fsanitize=address -fsanitize-coverage=edge,no-prune,trace-pc-guard"\
        CXXFLAGS="${CXXFLAGS} -g -fsanitize=address -fsanitize-coverage=edge,no-prune,trace-pc-guard"\
        CXX=/usr/bin/clang++-18 CC=/usr/bin/clang-18 bear -- ./script/cmake-build simulation\
        -DOT_FULL_LOGS=ON -DOT_THREAD_VERSION=1.3
fi
save_checkpoint && (( cur_step++ ))

## Apply patches and build BadThread
cd "$script_dir"
cd "$BADTHREAD_PATH"
if (( last_done_step < cur_step )); then
    echo "Step $cur_step: Applying patches to BadThread"
    git apply --ignore-whitespace ../patches/badthread.patch 
fi    
save_checkpoint && (( cur_step++ ))
if (( last_done_step < cur_step )); then
    echo "Step $cur_step: Building BadThread"
    CFLAGS="${CFLAGS} -g -fsanitize=address -fsanitize-coverage=edge,no-prune,trace-pc-guard"\
        CXXFLAGS="${CXXFLAGS} -g -fsanitize=address -fsanitize-coverage=edge,no-prune,trace-pc-guard"\
        CXX=/usr/bin/clang++-18 CC=/usr/bin/clang-18 bear -- ./script/cmake-build simulation\
        -DOT_FULL_LOGS=ON -DOT_THREAD_VERSION=1.3
fi
save_checkpoint && (( cur_step++ ))

## Build the Wireshark
cd "$script_dir"
if (( last_done_step < cur_step )); then
    echo "Step $cur_step: Building Wireshark"
    build_dir "$CUSTOM_WIRESHARK_PATH"
fi
save_checkpoint && (( cur_step++ ))

## Build the WDissector
cd "$script_dir"
if (( last_done_step < cur_step )); then
    echo "Step $cur_step: Building WDissector"
    build_dir "$WDISSECTOR_PATH"
fi
save_checkpoint && (( cur_step++ ))

## Build the ThreadFuzzer
cd "$script_dir"
if (( last_done_step < cur_step )); then
    echo "Step $cur_step: Building ThreadFuzzer"
    build_dir "."
fi
save_checkpoint && (( cur_step++ ))

echo "Setup script has successfully finished!"
rm -f "$STATE_FILE"