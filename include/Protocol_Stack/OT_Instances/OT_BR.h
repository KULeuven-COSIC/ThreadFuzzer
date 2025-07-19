#pragma once

#include <memory>
#include <string>

#include "Protocol_Stack/Packet_Generator/OT_packet_generator.h"
#include "DUT/OT_DUT.h"

#include <Protocol_Stack/RCP/RCP_Base.h>

class OT_BR final : public OT_Packet_Generator, public OT_DUT {
public:
    OT_BR(OT_TYPE ot_type);
    ~OT_BR() = default;
    bool start() override;
    bool stop() override;
    bool restart() override;
    bool is_running() override;
    bool reset() override;
    bool activate_thread() override;
    bool deactivate_thread() override;
  bool factoryreset() override;

private:
    const std::string name_ = "otbr-agent"; // NOTE: DO NOT CHANGE
    const std::string cli_name_ = "ot-ctl"; // NOTE: DO NOT CHANGE
    std::unique_ptr<RCP_Base> rcp_;
};
