#pragma once

#include <string>

#include "Protocol_Stack/Packet_Generator/OT_packet_generator.h"
#include "DUT/OT_DUT.h"

/* OpenThread Thread Device */
class OT_Dummy final : public OT_Packet_Generator, public OT_DUT {
public:
    ~OT_Dummy() = default;
    bool start() override;
    bool stop() override;
    bool restart() override;
    bool is_running() override;
    bool reset() override;
    bool activate_thread() override;
    bool deactivate_thread() override;
  bool factoryreset() override;
};
