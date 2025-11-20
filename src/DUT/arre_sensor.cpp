#include "DUT/arre_sensor.h"

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

bool Arre_Sensor::start() {
  // std::cerr << "ARRE: STARTING MEGA" << std::endl;
  my_logger_g.logger->debug("[DUT]: ARRE: STARTING");
  return helpers::echo_to_pipe("ON");
}

bool Arre_Sensor::stop() {
  // std::cerr << "ARRE: STOPPING MEGA" << std::endl;
  my_logger_g.logger->debug("[DUT]: ARRE: STOPPING");
  return helpers::echo_to_pipe("OFF");
}

bool Arre_Sensor::restart() {
  my_logger_g.logger->debug("[DUT]: ARRE: RESTARTING");
  std::cerr << "ARRE RESTARTING " << std::endl;
  if (!stop())
    return false;
  // std::this_thread::sleep_for(std::chrono::seconds(3));
  for (int i = 0; i < 3; i++) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "arre powered of for: " << i+1 << std::endl;
  }
  if (!start())
    return false;
  std::cout << "ARRE RESTART SUCCESS" << std::endl;
  return true;
}

bool Arre_Sensor::is_running() { return true; }

bool Arre_Sensor::reset() { return restart(); }

/**
   Purpose-built for the ARRE door & window sensor
 */
bool Arre_Sensor::factoryreset() {
  /* first make sure the device is actually turned on */
  // auto on = restart();
  // std::cout << "ARRE SHOULD BE RUNNING NOW " << std::endl;
  // std::this_thread::sleep_for(std::chrono::seconds(5));

  // helpers::chip_unpair(fuzz_strategy_config_g.chip_device_name);

  /* then reset */
  my_logger_g.logger->debug("[DUT]: ARRE: FR");
  std::cout << "ARRE STARTS FR" << std::endl;
  auto fr_on = helpers::echo_to_pipe("FR ON");
  std::this_thread::sleep_for(std::chrono::seconds(20));
  auto fr_off = helpers::echo_to_pipe("FR OFF");
  my_logger_g.logger->debug("[DUT]: ARRE: FR complete!");
  std::cout << "ARRE FINISHED FR" << std::endl;

  return fr_on && fr_off;
}

