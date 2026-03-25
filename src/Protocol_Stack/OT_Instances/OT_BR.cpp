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

  /* Start Boarder Router */
  // const std::string prepare_br_config_cmd =
  //     "echo \"OTBR_AGENT_OPTS='-I wpan0 -B " + interface +
  //     " spinel+hdlc+uart://" + socket + " trel://" + interface +
  //     "' \nOTBR_NO_AUTO_ATTACH=0\" | sudo tee /etc/default/" + name_;
  // if (helpers::exec_system_cmd_with_default_timeout(prepare_br_config_cmd) !=
  //     0) {
  //   my_logger_g.logger->error("Failed to prepare BR config");
  //   return false;
  // }
  // std::cout << "BR: prepared br config" << std::endl;

  // const std::string restart_br_service_cmd =
  //     "sudo service " + name_ + " restart";
  // if (helpers::exec_system_cmd_with_default_timeout(restart_br_service_cmd)
  // !=
  //     0) {
  //   my_logger_g.logger->error("Failed to restart BR");
  //   return false;
  // }
  std::cerr << "STARTING OTBR..." << std::endl;
  if (std::system("./third-party/ot-br-posix/script/server") != 0) {
    // if (helpers::exec_system_cmd_with_default_timeout(
    //         "./third-party/ot-br-posix/script/server &> /dev/null") != 0) {
    my_logger_g.logger->error("Failed to start BR server");
    return false;
  }
  std::cout << "BR: started server" << std::endl;

  // const std::string see_otbr_logs_cmd = "gnome-terminal
  // --geometry=180x25+200+550 --title=OTBR_LOGS -- bash -c \"tail -f
  // /var/log/syslog | grep " + name_ + "\""; if
  // (helpers::exec_system_cmd_with_default_timeout(see_otbr_logs_cmd) != 0) {
  //     my_logger_g.logger->error("Failed to launch a gnome terminal with OT_BR
  //     LOGS"); return false;
  // }




  /* Start RCP */
  if (!rcp_->start()) {
    my_logger_g.logger->error("Failed to start RCP");
    return false;
  }

  if (dynamic_cast<RCP_Sim *>(rcp_.get())) {
    /* Wait a bit if we are in the simulation mode */
    std::this_thread::sleep_for(std::chrono::seconds(2));
  }

  // const std::string factoryreset_cmd =
  //     "sudo " + cli_name_ + " factoryreset > /dev/null";
  // if (helpers::exec_system_cmd_with_default_timeout(factoryreset_cmd) != 0) {
  //   my_logger_g.logger->error("Failed to run \"factoryreset\" in {}",
  //                             cli_name_);
  //   return false;
  // }
  std::this_thread::sleep_for(std::chrono::seconds(2));
  std::system("ot-ctl factoryreset");
  std::this_thread::sleep_for(std::chrono::seconds(2));
  std::cout << "BR: factoryreset executed" << std::endl;

  std::string set_router_selection_jitter =
      cli_name_ + " routerselectionjitter " +
      std::to_string(timers_config_g.router_selection_jitter_s);
  // if (helpers::exec_system_cmd_with_default_timeout(
  //         set_router_selection_jitter) != 0) {
  //   my_logger_g.logger->error("Failed to set routerselectionjitter in BR");
  //   return false;
  // }
  std::system(set_router_selection_jitter.c_str());
  std::cout << "BR: set routerselectionjitter " << std::endl;
  std::string set_dataset_active_br_cmd =
      cli_name_ + " dataset set active " +
      technical_config_g.network_dataset;
  // if (helpers::exec_system_cmd_with_default_timeout(
  //         set_dataset_active_br_cmd) != 0) {
  //   my_logger_g.logger->error("Failed to set dataset active in BR");
  //   return false;
  // }
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

  // if (!this->deactivate_thread()) {
  //   my_logger_g.logger->warn("Failed to FR before stopping BR");
  //   success = false;
  // }

  // /* Kill log */
  // if (!helpers::kill_process("tail -f /var/log/syslog")) {
  //     my_logger_g.logger->warn("Failed to stop the BR logs");
  // }

  /* Stop RCP */
  if (!rcp_->stop()) {
    my_logger_g.logger->warn("Failed to stop RCP");
    success = false;
  }

  /* Stop Boarder Router */
  // if (!helpers::signal_service(name_, "SIGINT")) {
  //   my_logger_g.logger->warn("Failed to stop BR");
  //   std::cout << "FAILED TO STOP BR" << std::endl;
  //   success = false;
  // }
  std::cerr << "STOPPING OTBR" << std::endl;
  if (std::system("./third-party/ot-br-posix/script/server shutdown") != 0) {
    // if (helpers::exec_system_cmd_with_default_timeout(
    // "./third-party/ot-br-posix/script/server shutdown &> /dev/null") != 0) {
    my_logger_g.logger->error("Failed to start BR server");
    return false;
  }

  my_logger_g.logger->debug("BR application is stopped");

  return success;
}

bool OT_BR::restart() {
  my_logger_g.logger->debug("BR application gets restarted");

  factoryreset();
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

// bool OT_BR::restart() {
//   if (!deactivate_thread()) {
//       my_logger_g.logger->error("Failed to deactivate thread in BR");
//       return false;
//   }
//   std::this_thread::sleep_for(std::chrono::seconds(1));
//   if (!activate_thread()) {
//       my_logger_g.logger->error("Failed to activate thread in BR");
//       return false;
//   }
//   return true;
// }

bool OT_BR::activate_thread() {
  std::cout << "BR: gets activated" << std::endl;

  // const std::string ifconfig_up_cmd =
  //     "sudo " + cli_name_ + " ifconfig up > /dev/null";
  // if (helpers::exec_system_cmd_with_default_timeout(ifconfig_up_cmd) != 0) {
  //   my_logger_g.logger->error("Failed to run \"ifconfig up\" in {}", cli_name_);
  //   return false;
  // }
  std::system("ot-ctl ifconfig up");
  std::cout << "BR: ifconfig up" << std::endl;
  // const std::string thread_start =
  //     "sudo " + cli_name_ + " thread start" + " > /dev/null";
  // if (helpers::exec_system_cmd_with_default_timeout(thread_start) != 0) {
  //   my_logger_g.logger->error("Failed to run \"thread start\" in " + cli_name_);
  //   return false;
  // }
  std::system("ot-ctl thread start");
  std::cout << "BR: thread start" << std::endl;

  std::this_thread::sleep_for(std::chrono::seconds(2));

  my_logger_g.logger->debug("BR is configured and running");

  return true;
}

bool OT_BR::deactivate_thread() {

  // const std::string thread_stop =
  //     "sudo " + cli_name_ + " thread stop > /dev/null";
  // if (helpers::exec_system_cmd_with_default_timeout(thread_stop) != 0) {
  //   my_logger_g.logger->error("Failed to run \"thread start\" in " +
  //   cli_name_); return false;
  // }
  std::system("ot-ctl thread stop");
  std::cout << "BR: thread stop" << std::endl;
  // const std::string ifconfig_down_cmd =
  //     "sudo " + cli_name_ + " ifconfig down > /dev/null";
  // if (helpers::exec_system_cmd_with_default_timeout(ifconfig_down_cmd) != 0) {
  //   my_logger_g.logger->error("Failed to run \"ifconfig up\" in {}", cli_name_);
  //   return false;
  // }
  std::system("ot-ctl ifconfig down");
  std::cout << "BR: ifconfig down" << std::endl;

  // TODO: does this fix the factoryreset problem?
  // std::this_thread::sleep_for(std::chrono::seconds(2));

  my_logger_g.logger->debug("BR is stopped and factoryreset");

  return true;
}

bool OT_BR::factoryreset() { return true; }
