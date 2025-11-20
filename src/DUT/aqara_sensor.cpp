#include "DUT/aqara_sensor.h"

#include "Configs/Fuzzing_Settings/technical_config.h"
#include "Configs/Fuzzing_Settings/timers_config.h"
#include "helpers.h"
#include "my_logger.h"

#include <chrono>
#include <filesystem>
#include <iostream>
#include <thread>

extern My_Logger my_logger_g;
extern Technical_Config technical_config_g;
extern Timers_Config timers_config_g;

bool Aqara_Sensor::start() {
  // std::cerr << "AQARA: STARTING MEGA" << std::endl;
  my_logger_g.logger->debug("[DUT]: AQARA: STARTING");
  return helpers::echo_to_pipe("ON");
}

bool Aqara_Sensor::stop() {
  // std::cerr << "AQARA: STOPPING MEGA" << std::endl;
  my_logger_g.logger->debug("[DUT]: AQARA: STOPPING");
  return helpers::echo_to_pipe("OFF");
}

bool Aqara_Sensor::restart() {
  my_logger_g.logger->debug("[DUT]: AQARA: RESTARTING");
  std::cerr << "AQARA RESTARTING " << std::endl;
  if (!stop())
    return false;
  // std::this_thread::sleep_for(std::chrono::seconds(3));
  for (int i = 0; i < 3; i++) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "aqara powered of for: " << i+1 << std::endl;
  }
  if (!start())
    return false;
  std::cout << "AQARA RESTART SUCCESS" << std::endl;
  return true;
}

bool Aqara_Sensor::is_running() { return true; }

bool Aqara_Sensor::reset() { return restart(); }

/**
   Purpose-built for the AQARA door & window sensor
 */
bool Aqara_Sensor::factoryreset() {
  /* first make sure the device is actually turned on */
  // auto on = restart();
  // std::cout << "AQARA SHOULD BE RUNNING NOW " << std::endl;
  // std::this_thread::sleep_for(std::chrono::seconds(5));

  // helpers::chip_unpair(fuzz_strategy_config_g.chip_device_name);

  /* then reset */
  my_logger_g.logger->debug("[DUT]: AQARA: FR");
  std::cout << "AQARA STARTS FR" << std::endl;
  auto fr_on = helpers::echo_to_pipe("FR ON");
  std::this_thread::sleep_for(std::chrono::seconds(20));
  auto fr_off = helpers::echo_to_pipe("FR OFF");
  my_logger_g.logger->debug("[DUT]: AQARA: FR complete!");
  std::cout << "AQARA FINISHED FR" << std::endl;

  return fr_on && fr_off;
}
