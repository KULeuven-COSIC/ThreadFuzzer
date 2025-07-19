#include "Protocol_Stack/protocol_stack_factory.h"

#include "Protocol_Stack/protocol_stack_base.h"

#include "Protocol_Stack/dummy_stack.h"
#include "Protocol_Stack/openthread_stack.h"

std::unique_ptr<Protocol_Stack_Base> Protocol_Stack_Factory::get_protocol_stack_by_protocol_name(PROTOCOL_STACK_NAME protocol_stack_name)
{
    switch(protocol_stack_name) {
        case PROTOCOL_STACK_NAME::OPENTHREAD:
            return std::make_unique<OpenThread_Stack>();
        case PROTOCOL_STACK_NAME::DUMMY:
            return std::make_unique<Dummy_Stack>();
    }
    throw std::runtime_error("Unsupported protocol stack name");
}