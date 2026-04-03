#pragma once

#include <string>

#include "DUT/generic_sensor.h"

class Aqara_Sensor final : public Generic_Sensor {
public:
  Aqara_Sensor();
  ~Aqara_Sensor() = default;
};
