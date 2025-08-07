# ThreadFuzzer

This guide covers the installation of the complete ThreadFuzzer framework.

To specifically test AFL++ fuzzing for OpenThread, refer to the instructions in `AFL++_Comparison/README.md`.

## Installation Options

### Option 1: Native Installation (Ubuntu 22.04)

Run the setup script to install dependencies, pull submodules, and apply necessary patches:

```bash
chmod +x setup.sh && sudo ./setup.sh
```

### Option 2: Docker Installation

#### 1. Build the Docker Image

```bash
sudo docker build --pull --progress=plain -t thread_fuzzer:latest .
```

#### 2. Run the Container Interactively

```bash
sudo docker run --rm -it thread_fuzzer
```

> Inside the container, all commands should be run **without `sudo`**.

---

## Repository Structure

- `src/` — Source files  
- `include/` — Header files  
- `third-party/` — Third-party libraries  
- `common/` — Common shared libraries  
- `scripts/` — Utility scripts  
- `seeds/` — Crash reproduction seeds  
- `coverage_log/` — Coverage data from fuzzing runs  
- `logs/` — Logs from fuzzer runs  
- `configs/` — Configuration files:
  - `Fuzzing_Settings/` — Core fuzzer settings
  - `Fuzzing_Strategies/` — Fuzzing strategy configurations

---

## Running the Fuzzer in Simulation Mode

```bash
sudo ./build/ThreadFuzzer [MAIN CONFIG] [FUZZ STRATEGY 1] ... [FUZZ STRATEGY N]
```

### Example: Run Random Fuzzer

```bash
sudo ./build/ThreadFuzzer configs/Fuzzing_Settings/main_config.json configs/Fuzzing_Strategies/random_config.json
```

---

## Reproducing Crashes

To reproduce predefined crashes (1–6), replace `X` with the crash number:

```bash
sudo ./build/ThreadFuzzer seeds/crash_seeds/Crash_X/main_config.json seeds/crash_seeds/Crash_X/none_config.json
```

---

## Plotting Graphs from the Paper

> Note: This cannot be done from within a Docker container.

Use the appropriate script to generate figures:

- `./scripts/visualize_coverage_results_greybox.sh`
- `./scripts/visualize_coverage_results_blackbox.sh`
- `./scripts/visualize_coverage_results_tlv_fuzzer.sh`
- `./scripts/visualize_coverage_results_mtd.sh`

---

## Notes

### Working with WDissector

WDissector is buggy, unorganized, and potentially unsafe. Always run with AddressSanitizer enabled due to possible memory leaks.

To use custom Wireshark profiles, place them in the `bin/ws/` directory.