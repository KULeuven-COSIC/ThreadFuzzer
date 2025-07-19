#include <chrono>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "Communication/ot_pipe_helper.h"
#include "helpers.h"
#include "shm/fuzz_config.h"
#include "shm/shared_memory.h"

#include "Communication/shm_layer_communication.h"
#include "Communication/shm_layer_communication_factory.h"
#include "Configs/Fuzzing_Settings/main_config.h"
#include "Configs/Fuzzing_Settings/technical_config.h"
#include "Configs/Fuzzing_Settings/timers_config.h"
#include "Configs/Fuzzing_Strategies/fuzz_strategy_config.h"
#include "Coordinators/base_coordinator.h"
#include "Coordinators/coordinator_factory.h"
#include "DUT/DUT_names.h"
#include "Fuzzers/base_fuzzer.h"
#include "Fuzzers/fuzzer_factory.h"
#include "Protocol_Stack/openthread_stack.h"
#include "Protocol_Stack/protocol_stack_base.h"
#include "dissector.h"
#include "my_logger.h"
#include "packet.h"

extern Fuzz_Config fuzz_config_g;
extern Fuzz_Strategy_Config fuzz_strategy_config_g;
extern My_Logger my_logger_g;
extern Technical_Config technical_config_g;
extern Timers_Config timers_config_g;

void intHandler(int dummy) {
  my_logger_g.logger->warn("Called SIGINT handler!");
  SHM_Layer_Communication::is_active = 0;
}

void print_usage() {
  std::cout
      << "Usage: sudo ./build/ThreadFuzzer [PATH_TO_MAIN_CONFIG] "
         "[PATH_TO_FUZZ_STRATEGY_CONFIG_1] ... [PATH_TO_FUZZ_STRATEGY_CONFIG_N]"
      << std::endl;
}

int main(int argc, char *argv[]) {

  // Check if we are root
  if (getuid()) {
    std::cout << "Please run as a root" << std::endl;
    return -10;
  }

  // INITIALIZATION (START)
  std::cout << "Hello ThreadFuzzer" << std::endl;
  srand((unsigned)time(0));
  signal(SIGINT, intHandler);
  if (!parse_fuzz_config())
    return -1;

  /* Get main_config_path and save fuzzing strategies' config file names */
  std::string main_config_path;
  std::vector<std::string> fuzz_strategy_config_filenames;
  if (argc > 2) {
    main_config_path = std::string(argv[1]);
    fuzz_strategy_config_filenames.reserve(argc);
    for (int i = 2; i < argc; i++) {
      fuzz_strategy_config_filenames.push_back(std::string(argv[i]));
    }
  } else {
    print_usage();
    return -1;
  }

  /* Read fuzzing settings configs */
  if (!helpers::parse_json_file_into_class_instance(main_config_path,
                                                    main_config_g))
    return -1;

  if (main_config_g.technical_config_path.empty()) {
    std::cerr << "Please specify technical_config_path in main config"
              << std::endl;
    return -1;
  }
  if (!helpers::parse_json_file_into_class_instance(
          main_config_g.technical_config_path, technical_config_g))
    return -1;

  if (main_config_g.timers_config_path.empty()) {
    std::cerr << "Please specify timers_config_path in main config"
              << std::endl;
    return -1;
  }
  if (!helpers::parse_json_file_into_class_instance(
          main_config_g.timers_config_path, timers_config_g))
    return -1;

  /* Init logging. The base folder for logging would be log/TARGET_NAME */
  const std::filesystem::path log_dir_path =
      std::filesystem::path("logs") /
      std::filesystem::path(dut_name_to_string(main_config_g.dut_name)) /
      std::filesystem::path("run_" + helpers::get_current_time_stamp_short());
  if (!my_logger_g.init(log_dir_path)) {
    std::cerr << "Failed to init the logger" << std::endl;
    return -1;
  }

  /* Init the wdissector. For successful initialization, the wdissector library
   * demands having bin/ws directory. */
  if (!helpers::create_directories_if_not_exist("bin/ws")) {
    std::cerr << "Failed to create directory bin/ws" << std::endl;
    return -1;
  }
  if (!wdissector_init(NULL)) {
    std::cerr << "Wdissector init FAILED!" << std::endl;
    return -1;
  }
  wdissector_enable_fast_full_dissection(1);

  /* Read the fuzz strategy config */
  if (!helpers::parse_json_file_into_class_instance(
          fuzz_strategy_config_filenames.at(0), fuzz_strategy_config_g))
    return -1;

  std::unique_ptr<Base_Coordinator> coordinator =
      Coordinator_Factory::get_coordinator_by_name(
          main_config_g.coordinator_name);
  try {
    if (!coordinator->init(fuzz_strategy_config_filenames)) {
      my_logger_g.logger->error("Failed to init coordinator");
      throw;
    }
    coordinator->fuzzing_loop();
  } catch (const std::exception &ex) {
    my_logger_g.logger->warn("Caught an exception: {}", ex.what());
  }
  // INITIALIZATION (END)

  // DEINITIALIZATION (START)
  coordinator->deinit();
  std::cout << "Exiting ThreadFuzzer" << std::endl;
  // DEINITIALIZATION (END)

  return 0;
}
