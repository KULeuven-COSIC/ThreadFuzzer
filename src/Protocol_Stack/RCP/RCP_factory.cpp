#include "Protocol_Stack/RCP/RCP_factory.h"

#include "Protocol_Stack/RCP/RCP_dummy.h"
#include "Protocol_Stack/RCP/RCP_Sim.h"

#include <iostream>
#include <memory>

std::unique_ptr<RCP_Base> RCP_Factory::get_rcp_instance_by_name(RCP_NAME rcp_name) {
    switch(rcp_name) {
        case RCP_NAME::RCP_SIM:
            std::cout << "Created RCP_SIM" << std::endl;
            return std::make_unique<RCP_Sim>();
        case RCP_NAME::RCP_DUMMY:
            std::cout << "Created RCP_DUMMY" << std::endl;
            return std::make_unique<RCP_Dummy>();
        default: break;
    }
    throw std::runtime_error("Unknown RCP name");
}