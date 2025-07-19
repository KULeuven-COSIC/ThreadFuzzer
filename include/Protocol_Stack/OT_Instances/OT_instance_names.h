#pragma once

#include <nlohmann/json.hpp>

enum class OT_INSTANCE_NAME {
    OT_BR,
    OT_MTD,
    OT_FTD,
    OT_DUMMY
};

NLOHMANN_JSON_SERIALIZE_ENUM( OT_INSTANCE_NAME, {
    {OT_INSTANCE_NAME::OT_BR, "OT_BR"},
    {OT_INSTANCE_NAME::OT_MTD, "OT_MTD"},
    {OT_INSTANCE_NAME::OT_FTD, "OT_FTD"},
    {OT_INSTANCE_NAME::OT_DUMMY, "OT_DUMMY"},
})