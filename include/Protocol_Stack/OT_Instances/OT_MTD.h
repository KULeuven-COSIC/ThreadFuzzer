#pragma once

#include <string>

#include "Protocol_Stack/OT_Instances/OT_TD.h"

/* OpenThread Minimal Thread Device */
class OT_MTD final : public OT_TD {
public:
    OT_MTD(OT_TYPE ot_type);
    ~OT_MTD() = default;
    std::string get_name() const override;

private:
    const std::string name_ = "ot-cli-mtd"; // NOTE: DO NOT CHANGE
};