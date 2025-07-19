#include "Configs/Fuzzing_Settings/main_config.h"

#include "helpers.h"

#include <iostream>

Main_Config main_config_g;

std::ostream& Main_Config::dump(std::ostream& os) const {
    os << "DUT name: " << dut_name_to_string(dut_name) << std::endl;
    os << "asan_log_path: " << asan_log_path << std::endl;
    os << "Use monitor: " << std::boolalpha << use_monitor << std::endl;
    os << "RCP: " << static_cast<int>(rcp_name) << std::endl;
    return os;
}

std::ostream& operator<< (std::ostream& os, const Main_Config& config) {
    return config.dump(os);
}

