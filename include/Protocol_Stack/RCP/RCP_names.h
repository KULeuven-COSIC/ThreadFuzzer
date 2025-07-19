#pragma once

#include <nlohmann/json.hpp>

enum class RCP_NAME {
    RCP,
    RCP_SIM,
    RCP_DUMMY
};

NLOHMANN_JSON_SERIALIZE_ENUM( RCP_NAME, {
    {RCP_NAME::RCP, "RCP"},
    {RCP_NAME::RCP_SIM, "RCP_SIM"},
    {RCP_NAME::RCP_DUMMY, "RCP_DUMMY"}
})