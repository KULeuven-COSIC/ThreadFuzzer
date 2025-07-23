# ThreadFuzzer

## Installation
Run the setup script. It will install all the libraries, pull the submodules, checkout and apply the patches to them.
```bash
sudo ./setup.sh
```

## Running the Fuzzer in the simulation mode
```bash
sudo ./build/ThreadFuzzer configs/Fuzzing_Settings/main_config.json configs/Fuzzing_Strategies/random_config.json
```

## Notes
### Working with WDissector
WDissector's code is buggy, unorganized and often unpredictible. Be extra cautious using wdissector functions. Always run them with an address sanitizer, because some of them are leaking memory.
In order to set wireshark profiles to be used with WDissector, put them into the bin/ws folder.