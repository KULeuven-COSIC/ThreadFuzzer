#include "DUT/eve_sensor.h"

#include "Configs/Fuzzing_Settings/technical_config.h"
#include "Configs/Fuzzing_Settings/timers_config.h"
#include "helpers.h"
#include "my_logger.h"

#include <chrono>
#include <filesystem>
#include <iostream>
#include <ostream>
#include <thread>

extern My_Logger my_logger_g;
extern Technical_Config technical_config_g;
extern Timers_Config timers_config_g;

Eve_Sensor::Eve_Sensor() : Generic_Sensor("EVE", 20) {}