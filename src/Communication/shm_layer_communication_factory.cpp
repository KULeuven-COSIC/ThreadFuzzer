#include "Communication/shm_layer_communication_factory.h"

#include "Communication/shm_MLE_communication.h"
#include "Communication/shm_COAP_communication.h"

#include "shm/shared_memory.h"

#include <memory>
#include <stdexcept>

std::unique_ptr<SHM_Layer_Communication> SHM_Layer_Communication_Factory::get_shm_layer_communication_instance_by_layer_num(int layer_num) {

  EnumMutex layer_enum = static_cast<EnumMutex>(layer_num);

  switch (layer_enum) {
    case EnumMutex::SHM_MUTEX_MLE:
      return std::make_unique<SHM_MLE_Communication>();
    case EnumMutex::SHM_MUTEX_COAP:
      return std::make_unique<SHM_COAP_Communication>();
    default: break;
  }
    throw std::runtime_error("Failed to get the shm layer instance");
}
