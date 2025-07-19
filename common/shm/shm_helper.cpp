#include "shm_helper.h"
#include "shared_memory.h"
#include <iostream>

SHM_Helper::SHM_Helper(const std::string name, bool is_server, size_t buffer_size_per_layer) : name(name) {
    std::cerr << "SHM Helper Constructor START" << std::endl;
    shm = std::make_unique<SHM>(is_server, name, buffer_size_per_layer);
    std::cerr << "SHM Helper Constructor END" << std::endl;
}

// // TODO: change the direction & pdu_type type
// void SHM_Helper::send_mle_msg(PACKET_SRC packet_src, uint8_t* payload, uint32_t& payload_size) {
//     if (payload == NULL || payload_size == 0) return;
//     int layer = static_cast<int>(EnumMutex::SHM_MUTEX_MLE);
// 
// #if DEBUG_MODE
//     std::cerr << "Old payload: ";
//     print_bytes(payload, payload_size);
// #endif
// 
//     size_t payload_offset = 1;
//     std::unique_ptr<uint8_t[]> byte_buffer = std::make_unique<uint8_t[]>(SHM_MSG_MAX_SIZE);
// 
//     byte_buffer[0] = static_cast<uint8_t>(packet_src);
//     std::copy(payload, payload + payload_size, byte_buffer.get() + payload_offset);
// 
//     uint32_t bytes_to_send_size = payload_size + payload_offset;
//     shm->write_bytes(layer, byte_buffer.get(), bytes_to_send_size);
// 
//     uint32_t bytes_to_read_size;
//     if (!shm->read_bytes(layer, byte_buffer.get(), bytes_to_read_size)) {
//         return;
//     }
//     std::copy(byte_buffer.get() + payload_offset, byte_buffer.get() + bytes_to_read_size, payload);
//     payload_size = bytes_to_read_size - payload_offset;
// 
// #if DEBUG_MODE
//     std::cerr << "New payload: ";
//     print_bytes(payload, payload_size);
// #endif
// }
// 
// void SHM_Helper::send_coap_msg(PACKET_SRC packet_src, uint8_t *payload,
//                               uint32_t &payload_size) {
//     if (payload == NULL || payload_size == 0) return;
//     int layer = static_cast<int>(EnumMutex::SHM_MUTEX_COAP);
// 
// #if DEBUG_MODE
//     std::cerr << "Old payload: ";
//     print_bytes(payload, payload_size);
// #endif
// 
//     size_t payload_offset = 1;
//     std::unique_ptr<uint8_t[]> byte_buffer = std::make_unique<uint8_t[]>(SHM_MSG_MAX_SIZE);
// 
//     byte_buffer[0] = static_cast<uint8_t>(packet_src);
//     std::copy(payload, payload + payload_size, byte_buffer.get() + payload_offset);
// 
//     uint32_t bytes_to_send_size = payload_size + payload_offset;
//     shm->write_bytes(layer, byte_buffer.get(), bytes_to_send_size);
// 
//     uint32_t bytes_to_read_size;
//     if (!shm->read_bytes(layer, byte_buffer.get(), bytes_to_read_size)) {
//         return;
//     }
//     std::copy(byte_buffer.get() + payload_offset, byte_buffer.get() + bytes_to_read_size, payload);
//     payload_size = bytes_to_read_size - payload_offset;
// 
// #if DEBUG_MODE
//     std::cerr << "New payload: ";
//     print_bytes(payload, payload_size);
// #endif
// }

void SHM_Helper::send_msg(PACKET_SRC packet_src, uint8_t *payload,
                          uint32_t &payload_size, EnumMutex layer_mutex) {
    if (payload == NULL || payload_size == 0) return;
    int layer = static_cast<int>(layer_mutex);

#if DEBUG_MODE
    std::cerr << "Old payload: ";
    print_bytes(payload, payload_size);
#endif

    size_t payload_offset = 1;
    std::unique_ptr<uint8_t[]> byte_buffer = std::make_unique<uint8_t[]>(SHM_MSG_MAX_SIZE);

    byte_buffer[0] = static_cast<uint8_t>(packet_src);
    std::copy(payload, payload + payload_size, byte_buffer.get() + payload_offset);

    uint32_t bytes_to_send_size = payload_size + payload_offset;
    shm->write_bytes(layer, byte_buffer.get(), bytes_to_send_size);

    uint32_t bytes_to_read_size;
    if (!shm->read_bytes(layer, byte_buffer.get(), bytes_to_read_size)) {
        return;
    }
    std::copy(byte_buffer.get() + payload_offset, byte_buffer.get() + bytes_to_read_size, payload);
    payload_size = bytes_to_read_size - payload_offset;

#if DEBUG_MODE
    std::cerr << "New payload: ";
    print_bytes(payload, payload_size);
#endif
}
