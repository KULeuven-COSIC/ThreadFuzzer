#pragma once

#include <string>

class RCP_Base {
public:
    virtual ~RCP_Base() = default;
    virtual bool start() = 0;
    virtual bool stop() = 0;
    virtual bool restart() = 0;
    virtual bool is_running() = 0;
};