#include "fuzz_config.h"

#include <fstream>
#include <iostream>

Fuzz_Config fuzz_config_g;

bool Fuzz_Config::fuzz() const {
    return FUZZ_MLE || FUZZ_COAP;
}

bool parse_fuzz_config(const std::string& path_to_config)
{
    std::ifstream config_file(path_to_config);
    if (!config_file) {
        std::cerr << "Failed to open a fuzz config file: " << path_to_config << std::endl;
        return false;
    }

    nlohmann::json config_json;
    config_file >> config_json;
    fuzz_config_g = config_json.get<Fuzz_Config>();

    return true;
}
