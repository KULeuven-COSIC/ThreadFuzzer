#include "Protocol_Stack/Packet_Generator/OT_packet_generator_factory.h"

#include "Protocol_Stack/OT_Instances/OT_BR.h"
#include "Protocol_Stack/OT_Instances/OT_FTD.h"
#include "Protocol_Stack/OT_Instances/OT_MTD.h"
#include "Protocol_Stack/OT_Instances/OT_dummy.h"

#include "Configs/Fuzzing_Settings/technical_config.h"

extern Technical_Config technical_config_g;

std::unique_ptr<OT_Packet_Generator> OT_Packet_Generator_Factory::get_OT_packet_generator_by_name(OT_INSTANCE_NAME name) {
    switch (name) {
        case OT_INSTANCE_NAME::OT_BR:
            return std::make_unique<OT_BR>(OT_TYPE::PACKET_GENERATOR);
        case OT_INSTANCE_NAME::OT_MTD:
            return std::make_unique<OT_MTD>(OT_TYPE::PACKET_GENERATOR);
        case OT_INSTANCE_NAME::OT_FTD:
            return std::make_unique<OT_FTD>(OT_TYPE::PACKET_GENERATOR);
        case OT_INSTANCE_NAME::OT_DUMMY:
            return std::make_unique<OT_Dummy>();
    }
    throw std::runtime_error("Unknown OT instance name");
}