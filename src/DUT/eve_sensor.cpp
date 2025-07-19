#include "DUT/eve_sensor.h"

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

bool Eve_Sensor::start() {
  // std::cerr << "EVE: STARTING MEGA" << std::endl;
  my_logger_g.logger->debug("[DUT]: EVE: STARTING");
  return eve_to_pipe("ON");
}

bool Eve_Sensor::stop() {
  // std::cerr << "EVE: STOPPING MEGA" << std::endl;
  my_logger_g.logger->debug("[DUT]: EVE: STOPPING");
  return eve_to_pipe("OFF");
}

bool Eve_Sensor::restart() {
  my_logger_g.logger->debug("[DUT]: EVE: RESTARTING");
  std::cerr << "EVE RESTARTING " << std::endl;
  if (!stop())
    return false;
  // std::this_thread::sleep_for(std::chrono::seconds(3));
  for (int i = 0; i < 3; i++) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "eve powered of for: " << i+1 << std::endl;
  }
  if (!start())
    return false;
  std::cout << "EVE RESTART SUCCESS" << std::endl;
  return true;
}

void Eve_Sensor::prime_reboot_nb(int nb) { nb_of_reboots = nb; }

bool Eve_Sensor::is_running() { return true; }

bool Eve_Sensor::reset() { return restart(); }

/**
   Purpose-built for the EVE door & window sensor
 */
bool Eve_Sensor::factoryreset() {
  /* first make sure the device is actually turned on */
  // auto on = restart();
  // std::cout << "EVE SHOULD BE RUNNING NOW " << std::endl;
  // std::this_thread::sleep_for(std::chrono::seconds(5));

  // helpers::chip_unpair(fuzz_strategy_config_g.chip_device_name);

  /* then reset */
  my_logger_g.logger->debug("[DUT]: EVE: FR");
  std::cout << "EVE STARTS FR" << std::endl;
  auto fr_on = eve_to_pipe("FR ON");
  std::this_thread::sleep_for(std::chrono::seconds(20));
  auto fr_off = eve_to_pipe("FR OFF");
  my_logger_g.logger->debug("[DUT]: EVE: FR complete!");
  std::cout << "EVE FINISHED FR" << std::endl;

  return fr_on && fr_off;
}

bool Eve_Sensor::eve_to_pipe(const std::string cmd) {

  if (!std::filesystem::exists(technical_config_g.eve_pipe_name)) {
    my_logger_g.logger->error("The pipe {} does not exist",
                              technical_config_g.eve_pipe_name);
    return false;
  }

  // make sure following flags are disabled,
  // otherwise MEGA will reset w/o parsing the command we sent!
  system(("stty -F " + technical_config_g.eve_pipe_name +
          " -hupcl -brkint -icrnl -imaxbel -opost -isig -icanon -iexten -echo "
          "-echoe -echok -echoctl -echoke")
             .c_str());
  system(
      ("sudo echo \"" + cmd + "\"" + " > " + technical_config_g.eve_pipe_name)
          .c_str());

  return true;
}

std::string Eve_Sensor::ask_chip(const char *cmd) {
  std::array<char, 128> buffer;
  std::string result;
  std::unique_ptr<FILE, void (*)(FILE *)> pipe(popen(cmd, "r"),
                                               [](FILE *f) -> void {
                                                 // wrapper to ignore the return
                                                 // value from pclose() is
                                                 // needed with newer versions
                                                 // of gnu g++
                                                 std::ignore = pclose(f);
                                               });

  if (!pipe) {
    throw std::runtime_error("popen() failed!");
  }
  while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe.get()) !=
         nullptr) {
    result += buffer.data();
  }
  return result;
}
