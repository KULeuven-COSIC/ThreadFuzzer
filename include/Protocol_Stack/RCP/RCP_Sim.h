#pragma once

#include "Protocol_Stack/RCP/RCP_Base.h"

#include <string>

class RCP_Sim : public RCP_Base {
public:
    virtual ~RCP_Sim() = default;
    bool start();
    bool stop();
    bool restart();
    bool is_running();

private:
    const std::string name_ = "ot-rcp"; // NOTE: DO NOT CHANGE
};