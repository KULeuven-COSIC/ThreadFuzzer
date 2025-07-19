#pragma once

#include <string>

#include "Protocol_Stack/OT_Instances/base_OT_instance.h"

/* Base class for any OpenThread instance. It can also be a DUT for us, hence an inheritance from DUT_Base class. */
class OT_Packet_Generator : virtual public Base_OT_Instance {
public:
    virtual ~OT_Packet_Generator() = default;
    virtual bool start() = 0;
    virtual bool stop() = 0;
    virtual bool restart() = 0;
    virtual bool is_running() = 0;
    virtual bool reset() = 0;
    virtual bool activate_thread() = 0;
    virtual bool deactivate_thread() = 0;
};
