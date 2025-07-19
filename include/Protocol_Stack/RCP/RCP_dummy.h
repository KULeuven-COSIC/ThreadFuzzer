#pragma once

#include "Protocol_Stack/RCP/RCP_Base.h"

#include <string>

class RCP_Dummy : public RCP_Base {
public:
    virtual ~RCP_Dummy() = default;
    bool start();
    bool stop();
    bool restart();
    bool is_running();
};