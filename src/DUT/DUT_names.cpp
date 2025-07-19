#include "DUT/DUT_names.h"

std::string dut_name_to_string(DUT_NAME dut_name) {
    switch(dut_name) {
        case DUT_NAME::DUMMY:
            return "DUMMY";
        case DUT_NAME::OT_MTD:
            return "OT_MTD";
        case DUT_NAME::OT_FTD:
            return "OT_FTD";
        case DUT_NAME::OT_BR:
            return "OT_BR";
        case DUT_NAME::EVE_SENSOR:
            return "EVE_SENSOR";
        case DUT_NAME::NANOLEAF:
            return "NANOLEAF";
        case DUT_NAME::ALEXA:
            return "ALEXA";
    }
    return "UNKNOWN";
}
