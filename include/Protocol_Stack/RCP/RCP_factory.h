#pragma once

#include "Protocol_Stack/RCP/RCP_Base.h"
#include "Protocol_Stack/RCP/RCP_names.h"

#include <memory>

class RCP_Factory final {
public:
    static std::unique_ptr<RCP_Base> get_rcp_instance_by_name(RCP_NAME);
};