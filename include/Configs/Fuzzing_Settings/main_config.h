#pragma once

#include <iostream>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "Coordinators/coordinator_names.h"
#include "DUT/DUT_names.h"
#include "Protocol_Stack/OT_Instances/OT_instance_names.h"
#include "Protocol_Stack/RCP/RCP_names.h"
#include "Protocol_Stack/protocol_stack_names.h"

class Main_Config {
public:
  PROTOCOL_STACK_NAME protocol_stack_name = PROTOCOL_STACK_NAME::OPENTHREAD;
  OT_INSTANCE_NAME packet_generator_name = OT_INSTANCE_NAME::OT_BR;
  DUT_NAME dut_name = DUT_NAME::OT_MTD;
  RCP_NAME rcp_name = RCP_NAME::RCP_SIM;
  COORDINATOR_NAME coordinator_name;
  std::string asan_log_path = "/tmp";
  bool use_monitor = true;
  std::string dut_log_file = "";
  std::string gen_log_file = "";
  std::vector<std::string> dut_cli_args = {};
  std::vector<std::string> pg_cli_args = {};

  std::string technical_config_path =
      "";
  std::string timers_config_path =
      "";

public:
  std::ostream &dump(std::ostream &os) const;
};

std::ostream &operator<<(std::ostream &os, const Main_Config &path_config);

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
    Main_Config, protocol_stack_name, packet_generator_name, dut_name, rcp_name,
    coordinator_name, asan_log_path, use_monitor, dut_log_file, gen_log_file,
    dut_cli_args, pg_cli_args, technical_config_path, timers_config_path)
