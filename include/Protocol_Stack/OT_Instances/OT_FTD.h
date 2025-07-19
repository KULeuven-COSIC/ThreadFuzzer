#pragma once

#include <string>

#include "Protocol_Stack/OT_Instances/OT_TD.h"

/* OpenThread Full Thread Device */
class OT_FTD final : public OT_TD {
public:
    OT_FTD(OT_TYPE ot_type);
    ~OT_FTD() = default;
    std::string get_name() const override;

private:
    const std::string name_ = "ot-cli-ftd"; // NOTE: DO NOT CHANGE
};