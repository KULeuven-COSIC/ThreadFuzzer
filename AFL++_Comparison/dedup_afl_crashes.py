#!/usr/bin/env python3
import os
import subprocess
import argparse
import sys
from collections import defaultdict

# === CONFIGURATION ===
TIMEOUT = 5  # seconds

# === CRASH DEDUPLICATION ===
def get_crash_by_error(error):
    crash_dict = {
        "Crash 1": "Assertion `aMaxSize <= Address::kSize' failed",
        "Crash 2": "openthread/src/core/common/data.hpp:194:45",
        "Crash 3": "Assertion `(aRemoveLength <= mLength) && (GetBytes() <= removeStart) && (removeEnd <= end)' failed",
        "Crash 4": "Assertion `aDelay <= kMaxDelay' failed",
        "Crash 5": "Assertion `leader != nullptr' failed",
        "Crash 6": "openthread/src/core/net/ip6_address.cpp:69:5"
    }

    for crash, msg in crash_dict.items():
        if msg in error:
            return crash
    return "UNKNOWN CRASH"

def get_files_and_creation_times(directory):
    files = {}
    for file in os.listdir(directory):
        full_path = os.path.join(directory, file)
        if not os.path.isfile(full_path):
            continue
        time_created = os.path.getmtime(full_path)
        files[time_created] = file
    return files

def get_output(binary_path, seed_path):
    try:
        result = subprocess.run(
            [binary_path, seed_path],
            capture_output=True,
            timeout=TIMEOUT
        )
        out = result.stdout.decode(errors="ignore")
        err = result.stderr.decode(errors="ignore")
        total = (out + err).strip()
        return total
    except Exception as e:
        return f"ERROR: {str(e)}"

def main():
    parser = argparse.ArgumentParser(description="Deduplicate fuzz crashes.")
    parser.add_argument(
        "--crashes-dir", "-c", 
        required=True, 
        help="Path to AFL++ crash directory"
    )
    parser.add_argument(
        "--binary-path", "-b", 
        required=True, 
        help="Path to the fuzz target binary"
    )
    args = parser.parse_args()

    crashes_dir = args.crashes_dir
    binary_path = args.binary_path

    # === Auto-check paths ===
    if not os.path.isdir(crashes_dir):
        print(f"ERROR: Crash directory '{crashes_dir}' does not exist or is not a directory.", file=sys.stderr)
        sys.exit(1)

    if not os.path.isfile(binary_path) or not os.access(binary_path, os.X_OK):
        print(f"ERROR: Binary '{binary_path}' does not exist or is not executable.", file=sys.stderr)
        sys.exit(1)

    crash_outputs = defaultdict()
    files = get_files_and_creation_times(crashes_dir)
    sorted_files = dict(sorted(files.items()))

    if not sorted_files:
        print("No crash files found in the directory.")
        sys.exit(0)

    for filename in sorted_files.values():
        if not filename.startswith("id"):
            continue  # skip junk files
        print(f"Filename: {filename}")
        full_path = os.path.join(crashes_dir, filename)
        output = get_output(binary_path, full_path)
        crash = get_crash_by_error(output)
        if crash in crash_outputs:
            continue
        crash_outputs[crash] = filename

    print("\n=== Distinct Crash Outputs ===\n")
    for i, (c, seed) in enumerate(crash_outputs.items(), 1):
        print(f"[#{i}] {seed}\nOUTPUT:\n{c}\n{'-'*60}")

if __name__ == "__main__":
    main()
