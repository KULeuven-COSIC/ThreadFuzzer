#pragma once

#include "Protocol_Stack/protocol_stack_base.h"

#include "Protocol_Stack/Packet_Generator/OT_packet_generator.h"

#include <memory>
#include <string>

class OpenThread_Stack : public Protocol_Stack_Base {
public:
    OpenThread_Stack();
    virtual ~OpenThread_Stack() = default;
    bool start() override;
    bool stop() override;
    bool restart() override;
    bool is_running() override;
    bool reset() override;

private:
    std::unique_ptr<OT_Packet_Generator> packet_generator_;
};
