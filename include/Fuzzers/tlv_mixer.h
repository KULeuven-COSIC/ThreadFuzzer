#pragma once

#include "Fuzzers/base_fuzzer.h"

#include "packet.h"
#include "patch.h"

#include <string>
#include <memory>
#include <vector>

class TLV_Mixer : public Base_Fuzzer {
public:
    virtual ~TLV_Mixer() {}
    bool init() override;
    bool fuzz(Packet& packet) override;
};