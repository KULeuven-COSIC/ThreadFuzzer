#pragma once

#include "Fuzzers/random_fuzzer.h"

#include "packet.h"
#include "patch.h"

#include <string>
#include <unordered_map>
#include <memory>
#include <utility>
#include <vector>

/* Fuzzer that is designed to skip messages according to skip rules. */
class SkipFuzzer : public RandomFuzzer {
public:
    virtual ~SkipFuzzer() {}
    virtual bool fuzz(Packet& packet) override;
    virtual bool prepare_new_iteration() override;

private:
    void clean_packet_counter_map();
    
    std::unordered_map<std::string, std::size_t> packet_counter_map;
};
