# Using AFL++ to Fuzz OpenThread

Navigate to the comparison folder:

```bash
cd AFL++_Comparison
```

## Option 1: Native Installation

```bash
chmod +x setup_afl_cmp.sh && sudo ./setup_afl_cmp.sh
```

## Option 2: Docker Installation

### 1. Build the Docker Image

```bash
sudo docker build --pull --progress=plain -t openthread_afl_fuzz:latest .
```

### 2. Run the Container Interactively

```bash
sudo docker run --rm -it openthread_afl_fuzz
```

---

## Reproducing Paper Crashes

Use crash inputs from the `AFL++_Crash_Seeds` directory. Replace `X` with the crash number:

```bash
./openthread/build/simulation/tests/improved_fuzz/universal-mle-fuzzer-1 AFL++_Crash_Seeds/Universal_MLE_Fuzzer-1/Crash_X
```

---

## Running AFL++ Fuzzing

Two fuzzing harnesses are available in `AFL++_Comparison/openthread/tests/improved_fuzz/`:

- `universal_mle_fuzzer-1`: Simulates DUT joining a network controlled by a malicious device
- `universal_mle_fuzzer-2`: Simulates a malicious device trying to join DUT's network

Run AFL++ using the helper script (substitute `X` with 1 or 2):

```bash
chmod +x AFL++_Comparison/run_afl_fuzzing.sh
./AFL++_Comparison/run_afl_fuzzing.sh universal_mle_fuzzer-X
```

Seeds are taken from: `corpora/universal_mle_fuzzer-X/`  
Output is saved to: `universal_mle_fuzzer-X_out/`