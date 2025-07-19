#include "Coordinators/coordinator_factory.h"

#include "Coordinators/base_coordinator.h"
#include "Coordinators/dummy_coordinator.h"
#include "Coordinators/phys_timeout_based_coordinator.h"
#include "Coordinators/timeout_based_coordinator.h"

std::unique_ptr<Base_Coordinator> Coordinator_Factory::get_coordinator_by_name(
    COORDINATOR_NAME coordinator_name) {
  switch (coordinator_name) {
  case COORDINATOR_NAME::TIMEOUT_BASED:
    return std::make_unique<Timeout_Based_Coordinator>();
  case COORDINATOR_NAME::PHYS_TIMEOUT_BASED:
    return std::make_unique<Phys_Timeout_Based_Coordinator>();
  case COORDINATOR_NAME::DUMMY:
    return std::make_unique<Dummy_Coordinator>();
  default:
    break;
  }
  throw std::runtime_error("Cannot create coordinator: Unknown name");
}
