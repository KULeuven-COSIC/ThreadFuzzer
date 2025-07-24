# ThreadFuzzer

## Installation

Run the setup script to install dependencies, pull submodules, and apply necessary patches:

```bash
sudo ./setup.sh
```

---

## Repository Structure

- `src/` — Source files
- `include/` — Header files
- `third-party/` — Third-party libraries
- `common/` — Common libraries shared across modules
- `scripts/` — Multiple useful scripts
- `seeds/` — Seeds used to reproduce crashes
- `coverage_log/` — Coverage information generated during fuzzing runs
- `logs/` — Log files from fuzzer runs
- `configs/` — Configuration files:
  - `Fuzzing_Settings/` — Core fuzzer settings (technical configuration)
  - `Fuzzing_Strategies/` — Fuzzing strategy configurations

---

## Running the Fuzzer in Simulation Mode

To run the fuzzer, specify the main configuration file followed by one or more fuzzing strategy configuration files:

```bash
sudo ./build/ThreadFuzzer [MAIN CONFIG] [FUZZ STRATEGY 1] ... [FUZZ STRATEGY N]
```

### Example: Running the Random Fuzzer

```bash
sudo ./build/ThreadFuzzer configs/Fuzzing_Settings/main_config.json configs/Fuzzing_Strategies/random_config.json
```

---

## Reproducing Crashes

To reproduce one of the predefined crashes (1–6), use the corresponding configuration files. Replace `X` with the crash number:

```bash
sudo ./build/ThreadFuzzer configs/seeds/crash_seeds/Crash_X/main_config.json configs/seeds/crash_seeds/Crash_X/none_config.json
```

---

## Plotting the graphs from the paper
Run one of the following scripts to plot Figures 2, 3, 4 or 5, respectively:

- `./scripts/visualize_coverage_results_greybox.sh`
- `./scripts/visualize_coverage_results_blackbox.sh`
- `./scripts/visualize_coverage_results_tlv_fuzzer.sh`
- `./scripts/visualize_coverage_results_mtd.sh`


## Notes

### Working with WDissector

WDissector's code is buggy, unorganized, and often unpredictable. Use caution when invoking its functions. Always run them with an address sanitizer, as some components may leak memory.
To use custom Wireshark profiles with WDissector, place them in the `bin/ws/` directory.