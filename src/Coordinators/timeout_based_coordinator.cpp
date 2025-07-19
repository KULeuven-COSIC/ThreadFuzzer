#include "Coordinators/timeout_based_coordinator.h"

#include "Communication/shm_layer_communication.h"
#include "Configs/Fuzzing_Settings/technical_config.h"
#include "Configs/Fuzzing_Settings/timers_config.h"
#include "Configs/Fuzzing_Strategies/fuzz_strategy_config.h"
#include "Coordinators/base_coordinator.h"
#include "Fuzzers/base_fuzzer.h"
#include "helpers.h"
#include "my_logger.h"
#include "statistics.h"

#include <chrono>
#include <future>
#include <thread>

extern My_Logger my_logger_g;
extern Statistics statistics_g;
extern Fuzz_Strategy_Config fuzz_strategy_config_g;
extern Timers_Config timers_config_g;
extern Technical_Config technical_config_g;

Timeout_Based_Coordinator::Timeout_Based_Coordinator() {
  if (fuzz_strategy_config_g.use_coverage_logging ||
      fuzz_strategy_config_g.use_coverage_feedback)
    coverage_trackers.emplace_back(std::make_unique<Coverage_Tracker>(
        "PG_COVERAGE_TRACKER", "tcp://127.0.0.1:5567"));
  if (fuzz_strategy_config_g.use_coverage_logging ||
      fuzz_strategy_config_g.use_coverage_feedback)
    coverage_trackers.emplace_back(std::make_unique<Coverage_Tracker>(
        "DUT_COVERAGE_TRACKER", "tcp://127.0.0.1:5577"));
}

bool Timeout_Based_Coordinator::init(
    const std::vector<std::string> &fuzz_strategy_config_names) {
  if (!init_fuzzing_strategies(fuzz_strategy_config_names)) {
    my_logger_g.logger->error("Failed to init the fuzzing strategies");
    return false;
  }

  /* Remove old log files*/
  if (!main_config_g.dut_log_file.empty()) {
    if (int ret = remove(main_config_g.dut_log_file.c_str()); ret != 0) {
      my_logger_g.logger->debug(
          "Failed to remove old dut log file: {} (error code: {})",
          main_config_g.dut_log_file, ret);
    } else {
      my_logger_g.logger->debug("Successfully removed dut old log file: {}",
                                main_config_g.dut_log_file);
    }
  }

  if (!main_config_g.gen_log_file.empty()) {
    if (int ret = remove(main_config_g.gen_log_file.c_str()); ret != 0) {
      my_logger_g.logger->debug(
          "Failed to remove old gen log file: {} (error code: {})",
          main_config_g.gen_log_file, ret);
    } else {
      my_logger_g.logger->debug("Successfully removed old gen log file: {}",
                                main_config_g.gen_log_file);
    }
  }

  return true;
}

void Timeout_Based_Coordinator::deinit() {
  dut->stop();
  protocol_stack->stop();

  return;
}

void Timeout_Based_Coordinator::thread_dut_communication_func() {
  for (const Fuzz_Strategy_Config &fuzz_strategy_config :
       fuzz_strategy_configs_) {

    if (!prepare_new_fuzzing_sprint(fuzz_strategy_config)) {
      my_logger_g.logger->error("Failed to prepare new fuzzing sprint");
      break;
    }

    while (SHM_Layer_Communication::is_active) {

      my_logger_g.logger->info("================ START OF A NEW FUZZING "
                               "ITERATION {} ================",
                               global_iteration);
      my_logger_g.logger->flush();

      // NOTE: lets make this very simple: run the executable and check
      // only afterwards if DUT crashed this might disable the
      // fuzzing_stop_states, but oh, well.

      std::this_thread::sleep_for(
          std::chrono::milliseconds(timers_config_g.iteration_length_ms));

      if (!SHM_Layer_Communication::is_active)
        break;

      if (!renew_fuzzing_iteration()) {
        break;
      }
    }
    if (!SHM_Layer_Communication::is_active)
      break;
  }
  print_statistics();
  terminate_fuzzing();
}

bool Timeout_Based_Coordinator::renew_fuzzing_iteration() {
  my_logger_g.logger->info("Renewing fuzzing iteration");
  bool need_to_finish = false;

  if (!SHM_Layer_Communication::is_active)
    return false;

  /* Update the iteration */
  global_iteration++;
  local_iteration++;

  /* Update the statistics on the screen */
  // helpers::clear_screen();
  // print_statistics();
  std::cout << global_iteration << std::endl;

  wdissector_mutex.lock();

  /* Update coverage information and tweak probabilities */
  try {
    if (fuzz_strategy_config_g.use_coverage_logging ||
        fuzz_strategy_config_g.use_coverage_feedback)
      update_coverage_information();
    if (fuzz_strategy_config_g.use_coverage_feedback) update_probabilities(iteration_result.was_new_coverage_found);
  } catch (std::exception &ex) {
    my_logger_g.logger->warn("Exception during the coverage fetch: {}", ex.what());
  }

  Base_Fuzzer::mut_field_num_tracker.push_mutated_field_num(Base_Fuzzer::mutated_fields_num);

  if (Base_Fuzzer::mutated_fields_num == 0) {
    statistics_g.empty_iterations++;
  }

  my_logger_g.logger->debug("Number of mutated fields in this iteration: {}", Base_Fuzzer::mutated_fields_num);
  Base_Fuzzer::mutated_fields.clear();
  Base_Fuzzer::mutated_fields_num = 0;

  if (fuzz_strategy_config_g.use_probability_resets && Base_Fuzzer::mut_field_num_tracker.needs_reset()) {
      /* Reset the probabilities. This is done by deleting all the packets in the database.*/
      Base_Fuzzer::mut_field_num_tracker.reset();
      my_logger_g.logger->info("Resetting the probabilities");
      Base_Fuzzer::packet_field_tree_database.clear();
  }

  /* Check if any of the fuzzers requests finish of the fuzzing */
  for (size_t i = 0; i < fuzzers.size(); i++) {
    bool need_to_finish_local = !fuzzers[i]->prepare_new_iteration();
    need_to_finish |= need_to_finish_local;
    if (need_to_finish_local)
      my_logger_g.logger->info("Fuzzer indexed {} requested finishing fuzzing",
                               i);
  }

  need_to_finish |= (static_cast<int>(local_iteration) >=
                     (fuzz_strategy_config_g.total_iterations == -1
                          ? INT_MAX
                          : fuzz_strategy_config_g.total_iterations));

  stop_fuzzing_iteration = false;
  incoming_packet_num = 0;

  wdissector_mutex.unlock();

  /* Finish the fuzzing if needed */
  if (need_to_finish) {
    my_logger_g.logger->info("Finishing the fuzzing");
    return false;
  }

  /* check whether we can reset pg or not */
  if (!protocol_stack->reset()) {
    /* no restart, check after a while if thread has died */
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    if (!protocol_stack->is_running()) {
      statistics_g.protocol_stack_crash_counter++;
      std::cout << "RESET FAILED: PG IS DEAD" << std::endl;
      my_logger_g.logger->error("reset failed, pg is dead");
    } else {
      /* thread is alive, so failure! */
      std::cout << "RESET FAILED: PG IS ALIVE" << std::endl;
      my_logger_g.logger->error("reset failed, pg is alive");

      if (!protocol_stack->stop()) {
        std::cout << "FAILED to kill PG" << std::endl;
        my_logger_g.logger->error("failed to kill pg");
        return false;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(200));

      /* sanity check to check whether it actually stopped */
      if (protocol_stack->is_running()) {
        std::cout << "PG still running after stop" << std::endl;
        my_logger_g.logger->error("pg still running after stop");
        return false;
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    if (!protocol_stack->restart()) {
      std::cout << "FAILED to restart the DUT" << std::endl;
      my_logger_g.logger->error("failed to restart pg");
      return false;
    }
  }

  /* check whether we can restart dut or not */
  if (!dut->reset()) {
    /* no restart, check after a while if thread has died */
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    if (!dut->is_running()) {
      statistics_g.dut_crash_counter++;
      std::cout << "RESET FAILED: DUT IS DEAD" << std::endl;
      my_logger_g.logger->error("reset failed, dut is dead");
    } else { /* NOTE: thread is alive, but won't respond to reset! So restart */
      /* thread is alive, so failure! */
      std::cout << "RESET FAILED: DUT IS ALIVE" << std::endl;
      my_logger_g.logger->error("reset failed, dut is alive");

      if (!dut->stop()) {
        std::cout << "FAILED to kill DUT" << std::endl;
        my_logger_g.logger->error("failed to kill dut");
        return false;
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(200));

      /* sanity check to check whether it actually stopped */
      if (dut->is_running()) {
        std::cout << "DUT still running after stop" << std::endl;
        my_logger_g.logger->error("dut still running after stop");
        return false;
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    if (!dut->restart()) {
      std::cout << "FAILED to restart the DUT" << std::endl;
      my_logger_g.logger->error("failed to restart dut");
      return false;
    }
  }


  return true;
}

bool Timeout_Based_Coordinator::reset_target() { return true; }

std::string Timeout_Based_Coordinator::get_name() { return name_; }
