#pragma once

#include <nlohmann/json.hpp>

#define FUZZ_CONFIG_PATH "FUZZ_CONFIG_PATH_PLACEHOLDER"

class Fuzz_Config final {
public:
    bool FUZZ_MLE = false;
    bool FUZZ_COAP = false;

    bool fuzz() const;
};

bool parse_fuzz_config(const std::string& path_to_config = FUZZ_CONFIG_PATH);

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Fuzz_Config, FUZZ_MLE, FUZZ_COAP)
