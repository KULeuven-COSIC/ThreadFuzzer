#include "Coordinators/phys_timeout_based_coordinator.h"

#include "Communication/shm_layer_communication.h"
#include "Configs/Fuzzing_Settings/timers_config.h"
#include "helpers.h"
#include "my_logger.h"
#include "statistics.h"

#include <chrono>
#include <thread>

extern My_Logger my_logger_g;
extern Statistics statistics_g;
extern Timers_Config timers_config_g;

Phys_Timeout_Based_Coordinator::Phys_Timeout_Based_Coordinator() {
  if (fuzz_strategy_config_g.use_coverage_logging ||
      fuzz_strategy_config_g.use_coverage_feedback)
    coverage_trackers.emplace_back(std::make_unique<Coverage_Tracker>(
        "DUT_COVERAGE_TRACKER", "tcp://127.0.0.1:5577"));
  if (fuzz_strategy_config_g.use_coverage_logging ||
      fuzz_strategy_config_g.use_coverage_feedback)
    coverage_trackers.emplace_back(std::make_unique<Coverage_Tracker>(
        "OTBR_COVERAGE_TRACKER", "tcp://127.0.0.1:5567"));
}

bool Phys_Timeout_Based_Coordinator::init(
    const std::vector<std::string> &fuzz_strategy_config_names) {
  if (!init_fuzzing_strategies(fuzz_strategy_config_names)) {
    my_logger_g.logger->error("Failed to init the fuzzing strategies");
    return false;
  }

  /* put the device into pairing mode */
  bool start = dut->start();
  std::this_thread::sleep_for(std::chrono::seconds(5));
  bool reset = dut->factoryreset();

  return start && reset;
}

void Phys_Timeout_Based_Coordinator::deinit() { return; }

void Phys_Timeout_Based_Coordinator::thread_dut_communication_func() {
  std::cout << "starting communication" << std::endl;
  my_logger_g.logger->info("starting communication");

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

      int counter = timers_config_g.iteration_length_s;
      int iteration_time = 0;

      int current_silent_time = 0;

      int old_incoming_packet_num = 0;

      while (SHM_Layer_Communication::is_active && !stop_fuzzing_iteration &&
             counter--) {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        iteration_time = timers_config_g.iteration_length_s - counter;

        if (!protocol_stack->is_running()) {
          need_to_restart_protocol_stack = true;
          my_logger_g.logger->warn(
              "Protocol stack is not running. Stopping the fuzzing iteration.");
          break;
        }

        // NOTE: check whether dut is running at the end of each iteration, not
        // during the iteration if (!dut->is_running()) {
        //     need_to_restart_dut = true;
        //     my_logger_g.logger->warn("DUT is not running. Stopping the
        //     fuzzing iteration."); break;
        // }

        // nothing is send at all
        if (incoming_packet_num == 0 &&
            timers_config_g.empty_iteration_length_s <= iteration_time) {
          statistics_g.empty_iterations++;
          my_logger_g.logger->warn(
              "No incoming packets. Ending the iteration.");
          break;
        }

        // better: dut stays silent for empty_iteration_length_s
        if (incoming_packet_num == old_incoming_packet_num) {
          // statistics_g.long_silence++;
          current_silent_time++;
          my_logger_g.logger->warn("DUT stays silent...");
          // std::cout << "DUT stays silent for: " << current_silent_time
          //           << std::endl;
          // break;
        }
        if (incoming_packet_num == old_incoming_packet_num &&
            current_silent_time >= timers_config_g.empty_iteration_length_s) {
          // dut is silent for too long!!
          statistics_g.long_silence++;
          my_logger_g.logger->warn(
              "DUT IS VERY SLEEPY, Probably crashed, might restart");
          std::cout << " SLEEPY DUT!!! Probably crashed!!" << std::endl;
          break;
        }
        if (incoming_packet_num > old_incoming_packet_num) {
          // definitely not silent, so reset this counter!
          current_silent_time = 0;
          std::cout << " DUT alive and kicking! " << std::endl;
          old_incoming_packet_num = incoming_packet_num;
          // break;
        }
      }

      std::cout << "FINISHED ITERATION" << std::endl;

      my_logger_g.logger->info("Iteration time: {}", iteration_time);
      statistics_g.avg_iteration_time_s +=
          (static_cast<double>(iteration_time) -
           statistics_g.avg_iteration_time_s) /
          (global_iteration + 1);

      if (!SHM_Layer_Communication::is_active)
        break;

      std::cout << "STILL ACTIVE, NOW RENEWING" << std::endl;

      if (!renew_fuzzing_iteration()) {
        std::cout << "FAILED TO RENEW ITERATION" << std::endl;
        break;
      }
      std::cout << "RENEWING SUCCESS!" << std::endl;
    }
    if (!SHM_Layer_Communication::is_active)
      break;
  }
  terminate_fuzzing();
}

bool Phys_Timeout_Based_Coordinator::renew_fuzzing_iteration() {
  std::cout << "RENEWING THE ITERATION" << std::endl;
  my_logger_g.logger->info("Renewing fuzzing iteration");
  bool need_to_finish = false;

  /* Update the iteration */
  global_iteration++;
  local_iteration++;

  wdissector_mutex.lock();

  /* check whether the device has crashed */
  if (!dut->is_running()) {
    need_to_restart_dut += 1; //
  }

  std::cout << "DUT CHECK COMPLETE" << std::endl;

  /* Update the coverage information */
  try {
    if (fuzz_strategy_config_g.use_coverage_logging ||
        fuzz_strategy_config_g.use_coverage_feedback)
      update_coverage_information();
  } catch (std::exception &ex) {
    my_logger_g.logger->warn("Exception during the coverage fetch: {}",
                             ex.what());
    if (!protocol_stack->is_running()) {
      need_to_restart_protocol_stack = true;
    }
    if (!dut->is_running()) {
      need_to_restart_dut = true;
    }
  }

  /* Update the probabilities of the fields */
  if (fuzz_strategy_config_g.use_coverage_feedback)
    update_probabilities(iteration_result.was_new_coverage_found);

  wdissector_mutex.unlock();

  if (need_to_restart_dut) {
    statistics_g.dut_crash_counter++;
    my_logger_g.logger->warn("DUT has crashed!");
    // /* Get the crash reason if we log the DUT's screen */
    // if (!main_config_g.dut_log_file.empty()) {
    //     std::this_thread::sleep_for(std::chrono::seconds(1));
    //     std::string crash_info =
    //     helpers::read_file_last_chars(main_config_g.dut_log_file);
    //     my_logger_g.logger->info("DUT output:\n {}", crash_info);
    // }
  }

  if (need_to_restart_protocol_stack) {
    statistics_g.protocol_stack_crash_counter++;
    my_logger_g.logger->warn("Protocol Stack has crashed!");
    if (!main_config_g.gen_log_file.empty()) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
      std::string crash_info =
          helpers::read_file_last_chars(main_config_g.gen_log_file);
      my_logger_g.logger->info("PS output:\n {}", crash_info);
    }
  }

  /* Update the statistics on the screen */
  // helpers::clear_screen();
  print_statistics();

  std::cout << "PRINTED STATS" << std::endl;

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

  /* Finish the fuzzing if needed */
  if (need_to_finish) {
    my_logger_g.logger->info("Finishing the fuzzing");
    return false;
  }

  /* Prepare for the new iteration */
  if (need_to_restart_protocol_stack || !protocol_stack->reset()) {
    my_logger_g.logger->error(
        "Failed to reset a protocol stack. Restarting...");
    if (!protocol_stack->restart()) {
      my_logger_g.logger->error("Protocol stack cannot be restarted");
      return false;
    } else {
      my_logger_g.logger->warn("Protocol stack restarted successfully");
    }
  }

  /* End of epoch means factoryreset of the dut */
  if (statistics_g.epochs > epoch_cnt_) {
    std::cout << "EPOCH DONE, DOING FR INSTEAD OF RESET" << std::endl;
    /* every device has its custom way of entering pairing-mode */
    bool reset = dut->factoryreset();
    /* we don't want the br to interfere, so reset it */
    bool p_reset = protocol_stack->reset();
    /* then we re-pair the device */
    std::cout << "PAIRING the device using CHIP" << std::endl;
    helpers::chip_pair(fuzz_strategy_config_g.chip_device_name);
    if (!reset || !p_reset) return false;
    std::this_thread::sleep_for(std::chrono::seconds(1));
    epoch_cnt_ = statistics_g.epochs;
  } else if (global_iteration > 1 && (need_to_restart_dut || !dut->reset())) {
    my_logger_g.logger->warn("Failed to reset a DUT. Restarting...");
    if (!dut->restart()) {
      my_logger_g.logger->error("DUT cannot be restarted");
      return false;
    } else {
      my_logger_g.logger->warn("DUT restarted successfully");
    }
  } /* only restart when we are not in the first iteration, as factoryreset takes care of that */

  my_logger_g.logger->debug("Number of mutated fields in this iteration: {}",
                            Base_Fuzzer::mutated_fields_num);
  Base_Fuzzer::mutated_fields.clear();
  Base_Fuzzer::mutated_fields_num = 0;
  need_to_restart_protocol_stack = false;
  need_to_restart_dut = false;
  stop_fuzzing_iteration = false;
  incoming_packet_num = 0;

  return true;
}

bool Phys_Timeout_Based_Coordinator::reset_target() { return true; }

std::string Phys_Timeout_Based_Coordinator::get_name() { return name_; }
