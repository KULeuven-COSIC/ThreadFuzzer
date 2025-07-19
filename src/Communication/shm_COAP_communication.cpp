#include "Communication/shm_COAP_communication.h"

#include "packet.h"

#include "shm/shared_memory.h"

#include <memory>

SHM_COAP_Communication::SHM_COAP_Communication() : SHM_Layer_Communication(static_cast<int>(EnumMutex::SHM_MUTEX_MLE)) {}

void SHM_COAP_Communication::send(Packet &packet) {
    uint8_t* wrapped_pdu = packet.get_raw_data();
    uint32_t wrapped_pdu_size = packet.get_size();
    PACKET_SRC packet_src = packet.get_packet_src();
    int pdu_offset = 1; /* Offset value: for now only for fuzz_flag */

    uint32_t pdu_size = wrapped_pdu_size - dummy_headers_len - checksum_len;
    std::unique_ptr<uint8_t[]> msg = std::make_unique<uint8_t[]>(pdu_offset + pdu_size);
    msg[0] = static_cast<uint8_t>(packet_src);

    std::copy(wrapped_pdu + dummy_headers_len, wrapped_pdu + dummy_headers_len + pdu_size, msg.get() + pdu_offset);
    shm_->write_bytes(layer_, msg.get(), pdu_size + pdu_offset);
}

Packet SHM_COAP_Communication::receive() {
    std::unique_ptr<uint8_t[]> msg_ptr = std::make_unique<uint8_t[]>(SHM_MSG_MAX_SIZE);
    uint32_t read_msg_size;
    if (!shm_->read_bytes(layer_, msg_ptr.get(), read_msg_size)) {
        return {};
    }

    int pdu_offset = 1;
    int pdu_size = read_msg_size - pdu_offset;

    PACKET_SRC packet_src = static_cast<PACKET_SRC>(msg_ptr[0]);

    /* Wrap the payload into the dummy header */
    int wrapped_msg_len = dummy_headers_len + pdu_size + checksum_len;
    std::unique_ptr<uint8_t[]> wrapped_msg_ptr = std::make_unique<uint8_t[]>(wrapped_msg_len);

    /* Copy the headers*/
    std::copy(std::begin(dummy_headers), std::end(dummy_headers), wrapped_msg_ptr.get());

    /* Copy the payload */
    std::copy(msg_ptr.get() + pdu_offset, msg_ptr.get() + pdu_offset + pdu_size, wrapped_msg_ptr.get() + dummy_headers_len);

    /* Copy the checksum */
    std::copy(std::begin(checksum), std::end(checksum), wrapped_msg_ptr.get() + dummy_headers_len + pdu_size);
    
    return Packet(wrapped_msg_ptr.get(), wrapped_msg_len, layer_, packet_src);
}
