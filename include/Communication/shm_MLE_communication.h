#pragma once

#include "Communication/shm_layer_communication.h"

#include "packet.h"

class SHM_MLE_Communication final : public SHM_Layer_Communication {
public:
    SHM_MLE_Communication();
    void send(Packet& packet) override;
    Packet receive() override;

private:
    static constexpr uint8_t checksum[] = {0x00, 0x00};
    static constexpr int checksum_len = sizeof(checksum)/sizeof(checksum[0]);
    static constexpr uint8_t dummy_headers[] = {0x41, 0xd8, 0xcd, 0x3f, 0x66, 0xff, 0xff, 0x55, 0x9a,
                     0x9d, 0x60, 0x0e, 0xb9, 0x6e, 0xa6, 0x7f, 0x3b, 0x02,
                     0xf0, 0x4d, 0x4c, 0x4d, 0x4c, 0xa6, 0xbf, 0xff};
    static constexpr int dummy_headers_len = sizeof(dummy_headers)/sizeof(dummy_headers[0]);
};