#!/usr/bin/env bash

set -eo pipefail

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

readonly STATE_FILE=".setup-script.state"
readonly OPENTHREAD_PATH="${SCRIPT_DIR}/openthread"
readonly OPENTHREAD_CHECKOUT_TAG="thread-reference-20230706"

# Function to checkout a specific Git tag in a given repository path
function checkout_tag() {
    local repo_path="$1"
    local tag="$2"
    echo "Checking out tag $tag in repository $repo_path..."
    git -C "$repo_path" checkout "$tag"
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
}

function apt_install_all_packets {
    sudo apt update && sudo apt upgrade
    
    # GIT
    sudo apt install git 
    
    sudo apt install -y build-essential python3-dev automake cmake git flex bison libglib2.0-dev libpixman-1-dev python3-setuptools cargo libgtk-3-dev
    sudo apt install -y wget lsb-release software-properties-common gnupg
}

#################################################################################################################
cd ${SCRIPT_DIR}
if [[ -f "$STATE_FILE" ]]; then
    last_done_step=$(<"$STATE_FILE")
    echo "Staring from step: $last_done_step"
else
    last_done_step=0
    echo "Starting from the beginning"
fi

cur_step=1

# Install all apt packets
if (( last_done_step < cur_step )); then
    echo "Step $cur_step: Installing apt packets"
    apt_install_all_packets
fi
save_checkpoint && (( cur_step++ ))

# Install Clang
cd ${SCRIPT_DIR}
if (( last_done_step < cur_step )); then
    if ! command -v clang-18 &> /dev/null; then
        # If Clang is not installed
        echo "Step $cur_step: Installing clang"
        wget https://apt.llvm.org/llvm.sh && chmod +x llvm.sh && sudo ./llvm.sh 18 && rm llvm.sh
    fi
fi
save_checkpoint && (( cur_step++ ))

# Install AFL++
cd ${SCRIPT_DIR}
if ! command -v afl-clang-lto --version &> /dev/null; then
    if (( last_done_step < cur_step )); then
        echo "Step $cur_step: Cloning AFL++"
        git clone https://github.com/AFLplusplus/AFLplusplus
    fi
    save_checkpoint && (( cur_step++ ))

    if (( last_done_step < cur_step )); then
            # If afl-clang-lto is not installed
            echo "Step $cur_step: Install AFL++"
            cd AFLplusplus && make all && sudo make install
        fi
    fi
    save_checkpoint && (( cur_step++ ))
fi

# Make sure that the submodules are updated
# git submodule update --init --recursive
## For the anonymous Github: clone OpenThread 
cd ${SCRIPT_DIR}
if (( last_done_step < cur_step )); then
    echo "Step $cur_step: Cloning OpenThread"
    git clone https://github.com/openthread/openthread.git
fi
save_checkpoint && (( cur_step++ ))

# Checkout OpenThread
checkout_tag "$OPENTHREAD_PATH" "$OPENTHREAD_CHECKOUT_TAG"

# Apply the patch
if (( last_done_step < cur_step )); then
    echo "Step $cur_step: Apply OpenThread patch"
    cd ${OPENTHREAD_PATH}
    git apply --ignore-whitespace ${SCRIPT_DIR}/openthread.patch
fi
save_checkpoint && (( cur_step++ ))

# Build OpenThread
if (( last_done_step < cur_step )); then
    echo "Step $cur_step: Compile OpenThread using AFL++"
    cd ${SCRIPT_DIR}
    ./afl_fuzz_compile.sh afl-clang-lto
fi

echo "Setup script has successfully finished!"
rm -f "$STATE_FILE"