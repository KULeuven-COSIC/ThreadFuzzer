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

## Running Fuzzer on commercial devices

For pairing Matter devices, we use `chip-tool`. This means the Docker container needs access to Bluetooth via sharing `dbus` of the host and USB access to the RCP. Be careful to not restart `dbus` in the container as this might lead to all kinds of mayhem on the host.

Also, make sure that `mdns` is not running on the host to avoid a race condition on network interface.

```
sudo docker run -v /var/run/dbus/:/var/run/dbus/:z --privileged --name=threadfuzzer --rm -it thread_fuzzer
```

a slightly safer (non-privileged) way:

```
sudo docker run --security-opt apparmor=unconfined
                -v /var:/var \
                -v /proc:/proc \
                -v /run/dbus:/run/dbus \
                -v logs:/app/ThreadFuzzer/logs
                --device=/dev/net/tun \
                --device=/dev/ttyACM0 \
                --device=/dev/ttyUSB0 \
                --cap-add=NET_ADMIN \
                --cap-add=SYS_PTRACE \
                --name=threadfuzzer --rm -it thread_fuzzer
```

```
sudo docker build --pull --progress=plain -t thread_fuzzer:latest . && sudo docker run --security-opt apparmor=unconfined -v /var:/var -v /proc:/proc -v /run/dbus:/run/dbus -v logs:/app/ThreadFuzzer/logs --device=/dev/net/tun --device=/dev/ttyACM0 --device=/dev/ttyUSB0 --cap-add=NET_ADMIN --cap-add=SYS_PTRACE --name=threadfuzzer --rm -it thread_fuzzer
```


In here, `ttyACM0` is the RCP, `ttyUSB0` the controller for the physical device, `/dev/net/tun` and the volumes are needed for the otbr. `--cap-add` is needed for creating a dummy0 network interface. Note that this includes the otbr settings, therefore they persist after the container is shut down.

> Note for Ubuntu 24.04 If `apparmor` starts complaining about rsyslogd on the host (see `dmesg`), then disable it (on the host):

```
sudo ln -s /etc/apparmor.d/usr.sbin.rsyslogd /etc/apparmor.d/disable/
sudo apparmor_parser -R /etc/apparmor.d/usr.sbin.rsyslogd
``` 

> Also note that one might have to disable mDNS on the host to avoid conflicts with the one in the container. The reason is that we need access to dbus for working with the OpenThread RCP, so we share the directory, including mDNS specific ones. 


A provided entrypoint script will make sure parts are correctly set up and and shutdown (mainly `dbus` and `mdns`).

Note that attestation verification is bypassed to avoid downloading certificates for each commercial device. 

Example of pairing using `chip-tool`:

```
./connectedhomeip/out/chip-tool pairing ble-thread 6 hex:0e08000000000001000000030000174a0300001035060004001fffe00708fd1e234fcca6183b0c0402a0f7f80102dead0208dead1111dead2222030d4a616b6f6273506c617950656e051011112233445566778899dead1111dead0410209f8ccb50f556da46166ef4fdcbed4a 80049749 3070 --bypass-attestation-verifier true
```

For connecting and communication, the `ot-br-posix` project is used.

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

### Ensuring stability of fuzzing
Due to a known ASan issue (see [this Stack Overflow thread](https://stackoverflow.com/questions/78293129/c-programs-fail-with-asan-addresssanitizerdeadlysignal) for details), address space layout randomization (ASLR) should be disabled to ensure stable fuzzing runs.
Run the following command before starting the fuzzer, regardless of whether you are using Docker or a native setup:
```bash
echo 0 | sudo tee /proc/sys/kernel/randomize_va_space
```

### Working with WDissector

WDissector is buggy, unorganized, and potentially unsafe. Always run with AddressSanitizer enabled due to possible memory leaks.

To use custom Wireshark profiles, place them in the `bin/ws/` directory.
