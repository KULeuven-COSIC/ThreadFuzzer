#include "Protocol_Stack/OT_Instances/OT_FTD.h"

#include "helpers.h"

OT_FTD::OT_FTD(OT_TYPE ot_type) {
    ot_type_ = ot_type;
    session_name_ = helpers::get_name_prefix_by_ot_type(ot_type_) + get_name();
}

std::string OT_FTD::get_name() const {
    return name_;
}