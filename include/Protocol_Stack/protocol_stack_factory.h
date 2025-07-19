#pragma once

#include "Protocol_Stack/protocol_stack_base.h"
#include "Protocol_Stack/protocol_stack_names.h"

#include <memory>

class Protocol_Stack_Factory {
public:
    static std::unique_ptr<Protocol_Stack_Base> get_protocol_stack_by_protocol_name(PROTOCOL_STACK_NAME);
};