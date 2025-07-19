#pragma once

#include "shared_memory.h"

#include <cstdint>
#include <memory>
#include <cstddef>

class SHM_Helper {

public:
    SHM_Helper(const std::string name, bool is_server = false, size_t buffer_size_per_layer = SHM_MSG_MAX_SIZE);
  // void send_mle_msg(PACKET_SRC packet_src, uint8_t *payload,
  //                   uint32_t &payload_size);
  // void send_coap_msg(PACKET_SRC packet_src, uint8_t *payload,
  //                   uint32_t &payload_size);
  void send_msg(PACKET_SRC packet_src, uint8_t * payload, uint32_t &payload_size, EnumMutex layer);

  private:
    const std::string name;
    std::unique_ptr<SHM> shm;
};
