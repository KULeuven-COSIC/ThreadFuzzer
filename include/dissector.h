#pragma once

// #include "wdissector.h"

#include <cstdint>

#include <string>
#include <vector>

/**
 * Wrapper namespace over the wdissector
 */
namespace Dissector {
    std::string get_last_packet_protocol();
    std::string get_last_packet_summary();
    bool dissect_packet(std::vector<uint8_t>& packet);
    bool set_dissector(const std::string& dissector_name);
};
