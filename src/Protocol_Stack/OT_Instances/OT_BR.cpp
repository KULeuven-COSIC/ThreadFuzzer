#include "Protocol_Stack/OT_Instances/OT_BR.h"

#include "Configs/Fuzzing_Settings/main_config.h"
#include "Configs/Fuzzing_Settings/technical_config.h"
#include "Configs/Fuzzing_Settings/timers_config.h"
#include "Protocol_Stack/RCP/RCP_Sim.h"
#include "Protocol_Stack/RCP/RCP_factory.h"
#include "helpers.h"
#include "my_logger.h"

#include <chrono>
#include <iostream>
#include <thread>

extern Main_Config main_config_g;
extern Technical_Config technical_config_g;
extern Timers_Config timers_config_g;
extern My_Logger my_logger_g;

OT_BR::OT_BR(OT_TYPE ot_type) {
  rcp_ = RCP_Factory::get_rcp_instance_by_name(main_config_g.rcp_name);
  ot_type_ = ot_type;
}

bool OT_BR::start() {
  const std::string socket = technical_config_g.socket_2;
  const std::string interface = technical_config_g.interface;
  std::cerr << "STARTING OTBR..." << std::endl;
  if (std::system("./third-party/ot-br-posix/script/server") != 0) {
    // if (helpers::exec_system_cmd_with_default_timeout(
    //         "./third-party/ot-br-posix/script/server &> /dev/null") != 0) {
    my_logger_g.logger->error("Failed to start BR server");
    return false;
  }
  std::cout << "BR: started server" << std::endl;

  /* Start RCP */
  if (!rcp_->start()) {
    my_logger_g.logger->error("Failed to start RCP");
    return false;
  }

  if (dynamic_cast<RCP_Sim *>(rcp_.get())) {
    /* Wait a bit if we are in the simulation mode */
    std::this_thread::sleep_for(std::chrono::seconds(2));
  }

  std::this_thread::sleep_for(std::chrono::seconds(2));
  factoryreset();
  std::this_thread::sleep_for(std::chrono::seconds(2));
  std::cout << "BR: factoryreset executed" << std::endl;

  std::string set_router_selection_jitter =
      cli_name_ + " routerselectionjitter " +
      std::to_string(timers_config_g.router_selection_jitter_s);

  std::system(set_router_selection_jitter.c_str());
  std::cout << "BR: set routerselectionjitter " << std::endl;
  std::string set_dataset_active_br_cmd =
      cli_name_ + " dataset set active " +
      technical_config_g.network_dataset;

  std::system(set_dataset_active_br_cmd.c_str());
  std::cout << "BR: set dataset " << std::endl;

  if (!activate_thread()) {
    my_logger_g.logger->error("Failed to reset in BR start");
    return false;
  }

  my_logger_g.logger->debug("BR application is started");
  std::cout << "BR: is started" << std::endl;

  return true;
}

bool OT_BR::stop() {

  bool success = true;

  /* Stop RCP */
  if (!rcp_->stop()) {
    my_logger_g.logger->warn("Failed to stop RCP");
    success = false;
  }

  std::cerr << "STOPPING OTBR" << std::endl;
  if (std::system("./third-party/ot-br-posix/script/server shutdown") != 0) {
    my_logger_g.logger->error("Failed to start BR server");
    return false;
  }

  my_logger_g.logger->debug("BR application is stopped");

  const std::string cmd = "rm -rf /var/lib/thread/* /tmp/chip_*";
  my_logger_g.logger->debug("Deleting the cache with: {}", cmd);
  std::system(cmd.c_str());

  return success;
}

bool OT_BR::restart() {
  my_logger_g.logger->debug("BR application gets restarted");

  this->stop();
  std::this_thread::sleep_for(std::chrono::seconds(1));
  if (!this->start())
    return false;

  my_logger_g.logger->debug("BR application is restarted");

  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  return true;
}

bool OT_BR::is_running() {
  return helpers::is_process_alive(name_) && rcp_->is_running();
}

bool OT_BR::reset() {
  my_logger_g.logger->debug("BR application get reset");
  if (!deactivate_thread()) {
    my_logger_g.logger->error("Failed to deactivate thread in BR");
    return false;
  }
  std::this_thread::sleep_for(std::chrono::seconds(1));
  if (!activate_thread()) {
    my_logger_g.logger->error("Failed to activate thread in BR");
    return false;
  }
  my_logger_g.logger->debug("BR application is reset");
  return true;
}

bool OT_BR::activate_thread() {
  std::cout << "BR: gets activated" << std::endl;
  std::system("ot-ctl ifconfig up");
  std::cout << "BR: ifconfig up" << std::endl;
  std::system("ot-ctl thread start");
  std::cout << "BR: thread start" << std::endl;

  std::this_thread::sleep_for(std::chrono::seconds(2));

  my_logger_g.logger->debug("BR is configured and running");

  return true;
}

bool OT_BR::deactivate_thread() {
  std::system("ot-ctl thread stop");
  std::cout << "BR: thread stop" << std::endl;
  std::system("ot-ctl ifconfig down");
  std::cout << "BR: ifconfig down" << std::endl;

  my_logger_g.logger->debug("BR is stopped and factoryreset");

  return true;
}

bool OT_BR::factoryreset() {
  std::system("ot-ctl factoryreset");
  return true;
}
