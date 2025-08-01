#pragma once

#include <nlohmann/json.hpp>

enum class PROTOCOL_STACK_NAME {
    OPENTHREAD,
    DUMMY
};

NLOHMANN_JSON_SERIALIZE_ENUM( PROTOCOL_STACK_NAME, {
    {PROTOCOL_STACK_NAME::OPENTHREAD, "OPENTHREAD"},
    {PROTOCOL_STACK_NAME::DUMMY, "DUMMY"}
})