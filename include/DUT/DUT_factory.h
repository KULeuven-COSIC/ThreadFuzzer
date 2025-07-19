#pragma once

#include "DUT/DUT_names.h"
#include "DUT/DUT_base.h"
#include <memory>

class DUT_Factory {
public:
    static std::unique_ptr<DUT_Base> get_dut_by_name(DUT_NAME);
};