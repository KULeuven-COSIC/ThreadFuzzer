#pragma once

#include "Protocol_Stack/protocol_stack_base.h"

class Dummy_Stack : public Protocol_Stack_Base {
public:
    virtual ~Dummy_Stack() = default;
    bool start() override;
    bool stop() override;
    bool restart() override;
    bool is_running() override;
    bool reset() override;
};
