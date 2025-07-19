#pragma once

#include "Protocol_Stack/Packet_Generator/OT_packet_generator.h"
#include "Protocol_Stack/OT_Instances/OT_instance_names.h"

#include <memory>

class OT_Packet_Generator_Factory {
public:
    static std::unique_ptr<OT_Packet_Generator> get_OT_packet_generator_by_name(OT_INSTANCE_NAME);
};