#include "Coordinators/phys_timeout_based_coordinator.h"

#include "Communication/shm_layer_communication.h"
#include "Configs/Fuzzing_Settings/timers_config.h"
#include "DUT/DUT_names.h"
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
        "PG_COVERAGE_TRACKER", "tcp://127.0.0.1:5567"));
  /* no coverage available for real dut */
  // if (fuzz_strategy_config_g.use_coverage_logging ||
  //     fuzz_strategy_config_g.use_coverage_feedback)
  //   coverage_trackers.emplace_back(std::make_unique<Coverage_Tracker>(
  //       "DUT_COVERAGE_TRACKER", "tcp://127.0.0.1:5577"));
}

bool Phys_Timeout_Based_Coordinator::init(
    const std::vector<std::string> &fuzz_strategy_config_names) {
  if (!init_fuzzing_strategies(fuzz_strategy_config_names)) {
    my_logger_g.logger->error("Failed to init the fuzzing strategies");
    return false;
  }

  return true;
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

    /* put the device into pairing mode */
    // protocol_stack->stop();
    // dut->start(); // will wait for stability sake
    // dut->factoryreset(); // will put node in right state
    // std::cout << "DONE FACTORY RESET OF THE NODE MUHAHAHAHAA" << std::endl;//
    // protocol_stack->reset(); //
    // dut->start();
    // protocol_stack->start();
    dut->stop();
    protocol_stack->restart();
    dut->start();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    my_logger_g.logger->info("[COOR]: from here we can start fuzzing");
    std::cout << "[COOR]: here we start fuzzing" << std::endl;
    while (SHM_Layer_Communication::is_active) {
      

      if (local_iteration == 0 && fuzz_strategy_config_g.chip_recommissioning_step) {
        if (main_config_g.dut_name == DUT_NAME::NANOLEAF ||
            main_config_g.dut_name == DUT_NAME::EVE_SENSOR) {
          std::cout << "[COOR]: assumption that DUT is in pairing mode"
                    << std::endl;
        } else {
          std::cout << "[COOR]: giving you 60secs to put DUT in pairing mode"
                    << std::endl;
          std::this_thread::sleep_for(std::chrono::seconds(60));
        }

        if (helpers::chip_pair())
          terminate_fuzzing();

        reboot_count = helpers::chip_check_diagnostics();
        statistics_g.dut_reboot_counter = reboot_count;
        std::cout << "[COOR]: first rebootcount: " << reboot_count << std::endl;
	my_logger_g.logger->info("[COOR]: first rebootcount: {} ", reboot_count);

      }

      my_logger_g.logger->info("================ START OF A NEW FUZZING "
                               "ITERATION {} ================",
                               global_iteration);
      my_logger_g.logger->flush();

      int counter = timers_config_g.iteration_length_s;

      /* very shady trick to make the snd iteration waay shorter */
      // if (global_iteration == 1) {
      //   std::cerr << "warning: running shorter iteration" << std::endl;
      //   counter = 60;
      // }

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

  /* NOTE: from here no packets can flow! */
  wdissector_mutex.lock();

  /* check whether the device has crashed */
  if (!dut->is_running()) {
    need_to_restart_dut = true; //
  }

  std::cout << "DUT CHECK COMPLETE" << std::endl;

  /* Update the coverage information */
  try {
    if (fuzz_strategy_config_g.use_coverage_logging ||
        fuzz_strategy_config_g.use_coverage_feedback) {
      update_coverage_information();
      /* Update the probabilities of the fields */
      if (fuzz_strategy_config_g.use_coverage_feedback)
        update_probabilities(iteration_result.was_new_coverage_found);
    }
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

  Base_Fuzzer::mut_field_num_tracker.push_mutated_field_num(
      Base_Fuzzer::mutated_fields_num);

  if (Base_Fuzzer::mutated_fields_num == 0) {
    statistics_g.empty_iterations++;
  }

  my_logger_g.logger->debug("Number of mutated fields in this iteration: {}",
                            Base_Fuzzer::mutated_fields_num);
  Base_Fuzzer::mutated_fields.clear();
  Base_Fuzzer::mutated_fields_num = 0;

  if (fuzz_strategy_config_g.use_probability_resets &&
      Base_Fuzzer::mut_field_num_tracker.needs_reset()) {
    /* Reset the probabilities. This is done by deleting all the packets in the
     * database.*/
    Base_Fuzzer::mut_field_num_tracker.reset();
    my_logger_g.logger->info("Resetting the probabilities");
    Base_Fuzzer::packet_field_tree_database.clear();
  }

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

  bool need_to_perform_clean_attach = false;
  /* Check if any of the fuzzers requests finish of the fuzzing */
  for (size_t i = 0; i < fuzzers.size(); i++) {
    int need_to_finish_local = fuzzers[i]->prepare_new_iteration();
    need_to_finish |= !need_to_finish_local;
    if (need_to_finish_local == 0) {
      my_logger_g.logger->info("Fuzzer indexed {} requested finishing fuzzing",
                               i);
      print_statistics();
    }
    // check if we need to schedule a hard reset
    if (need_to_finish_local == 2) {
      need_to_perform_clean_attach = true;
    }
  }

  need_to_finish |= (static_cast<int>(local_iteration) >=
                     (fuzz_strategy_config_g.total_iterations == -1
                          ? INT_MAX
                          : fuzz_strategy_config_g.total_iterations));

  /* NOTE: after this, packets can again flow */
  wdissector_mutex.unlock();

  /* NOTE: here we'll fetch the reboot count outside the lock, as it otherwise
   * breaks */
  if (need_to_perform_clean_attach) {
    /* fetch the reboot count */
    std::cout << "[COOR]: fetchin current_reboot_count"  << std::endl;
    my_logger_g.logger->info("[COOR]: fetching current_reboot_count!");
    int current_reboot_count = helpers::chip_check_diagnostics();
    std::cout << "[COOR]: rbtcnt " << current_reboot_count << std::endl;
    bool spurrious_reboot = (current_reboot_count - reboot_count) !=
                            (1 + fuzz_strategy_config_g.epoch_size);
    if (spurrious_reboot) {
      std::cout << "[COOR]: crash!" << std::endl;
      my_logger_g.logger->info("[COOR]: crash!");
      std::cout << "[COOR]: expected: " << 1 + fuzz_strategy_config_g.epoch_size
                << " but was " << current_reboot_count - reboot_count
                << std::endl;
      my_logger_g.logger->info("[COOR]: expected {} but was {}",
                               1 + fuzz_strategy_config_g.epoch_size,
                               current_reboot_count - reboot_count);
      statistics_g.dut_crash_counter++;
    }
    current_reboot_count = reboot_count;

    bool pstop = protocol_stack->stop();
    bool start = dut->start();
    std::this_thread::sleep_for(std::chrono::seconds(10));
    my_logger_g.logger->debug("WE ARE HERE NOW");
    bool reset = dut->factoryreset();
    bool pstart = protocol_stack->start();
    if (helpers::chip_pair() == 0) {
      statistics_g.dut_reboot_counter = helpers::chip_check_diagnostics();
      std::cout << "AND HERE WE ARE DONE!" << std::endl;
      if (!(start && reset && pstart && pstop)) {
        my_logger_g.logger->error("scheduling a hard reset failed!");
        need_to_finish = true;
      }
    } else {
      need_to_finish = true;
    }
  }

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
      // reset is performed in restart already!
      // if (!protocol_stack->reset()) {
      //   my_logger_g.logger->error(
      //       "Protocol stack cannot be reconfigured after restart");
      //   return false;
      // }
      my_logger_g.logger->warn(
          "Protocol stack reconfigured successfully after restart");
    }
  }

  /* End of epoch means factoryreset of the dut */
  // if (statistics_g.epochs > epoch_cnt_) {
  //   std::cout << "EPOCH DONE, DOING FR INSTEAD OF RESET" << std::endl;
  //   /* every device has its custom way of entering pairing-mode */
  //   bool reset = dut->factoryreset();
  //   /* we don't want the br to interfere, so reset it */
  //   bool p_reset = protocol_stack->reset();
  //   /* then we re-pair the device */
  //   std::cout << "PAIRING the device using CHIP" << std::endl;
  //   helpers::chip_pair();
  //   if (!reset || !p_reset)
  //     return false;
  //   std::this_thread::sleep_for(std::chrono::seconds(1));

  //   /* reset the dut just in case */
  //   dut->reset();

  //   epoch_cnt_ = statistics_g.epochs;
  // } else
  if ((need_to_restart_dut || !dut->reset())) {
    my_logger_g.logger->warn("Failed to reset a DUT. Restarting...");
    if (!dut->restart()) {
      my_logger_g.logger->error("DUT cannot be restarted");
      return false;
    } else {
      my_logger_g.logger->warn("DUT restarted successfully");
    }
  } /* only restart when we are not in the first iteration, as factoryreset
       takes care of that */

  need_to_restart_protocol_stack = false;
  need_to_restart_dut = false;
  stop_fuzzing_iteration = false;
  incoming_packet_num = 0;

  return true;
}

bool Phys_Timeout_Based_Coordinator::reset_target() { return true; }

std::string Phys_Timeout_Based_Coordinator::get_name() { return name_; }
