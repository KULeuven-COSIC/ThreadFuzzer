#pragma once

#include "Fuzzers/base_fuzzer.h"

#include "packet.h"

/* LenFuzzer is responsible for constructing 
   the packet using information from its field tree.
   This is a prototype for future smarter fuzzing techniques. */
class LenFuzzer : public Base_Fuzzer {
public:
    virtual ~LenFuzzer() {}
    bool init() override;
    bool fuzz(Packet& packet) override;
};