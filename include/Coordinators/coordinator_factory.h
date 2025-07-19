#pragma once

#include "Coordinators/base_coordinator.h"
#include "Coordinators/coordinator_names.h"

#include <memory>

class Coordinator_Factory {
public:
    static std::unique_ptr<Base_Coordinator> get_coordinator_by_name(COORDINATOR_NAME);
};
