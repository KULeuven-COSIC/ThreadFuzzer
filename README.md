# ThreadFuzzer

## Installation
### Step 1
Clone the repository with all the submodules:
```bash
git clone --recurse-submodules https://github.com/IljaSir/ThreadFuzzer
```
If you’ve already cloned the repository without the submodules, you can update them by running:
```bash
git submodule update --init --recursive
```

### Step 2
Run the setup script. It will checkout the submodules and apply the patches.
```bash
./setup.sh
```

## Notes
### Working with WDissector
WDissector's code is buggy, unorganized and often unpredictible. Be extra cautious using wdissector functions. Always run them with an address sanitizer, because some of them are leaking memory.
In order to set wireshark profiles to be used with WDissector, put them into the bin/ws folder.