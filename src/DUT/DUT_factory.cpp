#include "DUT/DUT_factory.h"

#include "DUT/DUT_names.h"
#include "DUT/alexa.h"
#include "DUT/dummy_DUT.h"
#include "DUT/eve_sensor.h"
#include "DUT/nanoleaf.h"
#include "Protocol_Stack/OT_Instances/OT_BR.h"
#include "Protocol_Stack/OT_Instances/OT_FTD.h"
#include "Protocol_Stack/OT_Instances/OT_MTD.h"

#include "Configs/Fuzzing_Settings/technical_config.h"

extern Technical_Config technical_config_g;

std::unique_ptr<DUT_Base> DUT_Factory::get_dut_by_name(DUT_NAME dut_name) {
  switch (dut_name) {
  case DUT_NAME::DUMMY:
    return std::make_unique<Dummy_DUT>();
  case DUT_NAME::ALEXA:
    return std::make_unique<Alexa>();
  case DUT_NAME::NANOLEAF:
    return std::make_unique<Nanoleaf>();
  case DUT_NAME::EVE_SENSOR:
    return std::make_unique<Eve_Sensor>();
  case DUT_NAME::OT_MTD:
    return std::make_unique<OT_MTD>(OT_TYPE::DUT);
  case DUT_NAME::OT_FTD:
    return std::make_unique<OT_FTD>(OT_TYPE::DUT);
  case DUT_NAME::OT_BR:
    return std::make_unique<OT_BR>(OT_TYPE::DUT);
  }
  throw std::runtime_error("Unsupported dut name");
}
