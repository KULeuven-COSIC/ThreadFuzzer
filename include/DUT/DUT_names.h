#pragma once

#include <nlohmann/json.hpp>

enum class DUT_NAME {
    OT_MTD,
    OT_FTD,
    OT_BR,
    ALEXA,
    NANOLEAF,
    EVE_SENSOR,
    DUMMY
};

NLOHMANN_JSON_SERIALIZE_ENUM(DUT_NAME, {{DUT_NAME::OT_MTD, "OT_MTD"},
                                        {DUT_NAME::OT_FTD, "OT_FTD"},
                                        {DUT_NAME::NANOLEAF, "NANOLEAF"},
                                        {DUT_NAME::EVE_SENSOR, "EVE_SENSOR"},
                                        {DUT_NAME::OT_BR, "OT_BR"},
                                        {DUT_NAME::ALEXA, "ALEXA"},
                                        {DUT_NAME::DUMMY, "DUMMY"}})

std::string dut_name_to_string(DUT_NAME);
