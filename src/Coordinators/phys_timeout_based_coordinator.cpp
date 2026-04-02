#include "Coordinators/phys_timeout_based_coordinator.h"

#include "Communication/shm_layer_communication.h"
#include "Configs/Fuzzing_Settings/timers_config.h"
#include "Coordinators/base_coordinator.h"
#include "DUT/DUT_names.h"
#include "fuzz_config.h"
#include "helpers.h"
#include "my_logger.h"
#include "statistics.h"

#include "Communication/shm_layer_communication_factory.h"

#include <chrono>
#include <thread>

extern My_Logger my_logger_g;
extern Statistics statistics_g;
extern Timers_Config timers_config_g;
extern Fuzz_Config fuzz_config_g;

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

    my_logger_g.logger->info("[COOR]: from here we can start fuzzing");
    std::cout << "[COOR]: here we start fuzzing" << std::endl;
    while (SHM_Layer_Communication::is_active) {

      if (local_iteration == 0 &&
          fuzz_strategy_config_g.chip_recommissioning_step) {
        if (main_config_g.dut_name == DUT_NAME::NANOLEAF ||
            main_config_g.dut_name == DUT_NAME::EVE_SENSOR) {
          std::cout << "[COOR]: assumption that DUT is in pairing mode"
                    << std::endl;
        } else {
          std::cout << "[COOR]: giving you 60secs to put DUT in pairing mode"
                    << std::endl;
          std::this_thread::sleep_for(std::chrono::seconds(60));
        }

        if (helpers::chip_pair(statistics_g.epochs + 1)) {
          std::cout << "[COOR]: ERROR pairing DUT again failed" << std::endl;
          my_logger_g.logger->error("[COOR]: ERROR pairing DUT again failed");
          terminate_fuzzing();
        }

        /* NOTE: we need an additional offset if we run using the modulo trick
         */
        reboot_count = helpers::chip_check_diagnostics(statistics_g.epochs + 1);
        statistics_g.dut_reboot_counter = reboot_count;
        std::cout << "[COOR]: first rebootcount: " << reboot_count << std::endl;
        my_logger_g.logger->info("[COOR]: first rebootcount: {} ",
                                 reboot_count);

        // give the fuzzer_loop the powers back
        is_epoch_it = false;

        if (!protocol_stack->reset()) {
          std::cout << "[COOR]: cannot restart protocol stack, exiting"
                    << std::endl;
          my_logger_g.logger->error(
              "[COOR]: cannot restart protocol stack, exiting");
          terminate_fuzzing();
        }

        dut->reset();
      }

      my_logger_g.logger->info("================ START OF A NEW FUZZING "
                               "ITERATION {} ================",
                               global_iteration);
      my_logger_g.logger->flush();

      int counter = timers_config_g.iteration_length_s;

      /* very shady trick to make the snd iteration waay shorter */
      int epoch_resizer = 20;
      if (local_iteration % fuzz_strategy_config_g.epoch_size == 1 &&
          local_iteration != 0) {
        std::cerr << "warning: running shorter iteration" << std::endl;
        if (counter < epoch_resizer) {
          std::cerr << "iteration is already short lengthening it" << std::endl;
        }
        counter = epoch_resizer;
      }

      int current_silent_time = 0;
      int old_incoming_packet_num = 0;
      int iteration_time = 0;

      while (SHM_Layer_Communication::is_active && !stop_fuzzing_iteration &&
             counter--) {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        iteration_time++;

        if (!protocol_stack->is_running()) {
          need_to_restart_protocol_stack = true;
          my_logger_g.logger->warn(
              "Protocol stack is not running. Stopping the fuzzing iteration.");
          break;
        }

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

        /* TODO: tidy this up */
        if (advertisement_counter > 100) {
          std::cout << "ending this iteration early" << std::endl;
          my_logger_g.logger->info("ending this iteration early");
          break;
        }
      }

      std::cout << "FINISHED ITERATION" << std::endl;

      my_logger_g.logger->info("Iteration time: {}", iteration_time);
      statistics_g.avg_iteration_time_s +=
          (static_cast<double>(iteration_time) -
           statistics_g.avg_iteration_time_s) /
          (global_iteration + 1);
      iteration_time = 0;

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

  /* reset advertisement counter */
  advertisement_counter = 0;

  /* disable fuzzing at end of the epoch for checking/pairing */
  if (local_iteration % fuzz_strategy_config_g.epoch_size ==
          fuzz_strategy_config_g.epoch_size - 1 &&
      !is_epoch_it) {
    std::cout << "[COOR]: INTO EPOCH_IT" << std::endl;
    is_epoch_it = true;
    statistics_g.epochs++;
    statistics_g.it_in_epochs = 0;
  } else {
    statistics_g.it_in_epochs++;
  }
  my_logger_g.logger->info("iteration is epoch_it {}", is_epoch_it);
  std::cout << "iteration is an epoch_it " << is_epoch_it << std::endl;

  /* we ran an epoch iteration */
  if (local_iteration % fuzz_strategy_config_g.epoch_size == 0 &&
      local_iteration >= fuzz_strategy_config_g.epoch_size) {
    if (!is_epoch_it) {
      need_to_finish = true;
      std::cout << "[COOR]: ERROR, we should be in epoch_it at this point"
                << std::endl;
      my_logger_g.logger->error(
          "[COOR]: ERROR, we should be in epoch_it at this");
    }
    /* fetch the reboot count */
    std::cout << "[COOR]: fetchin current_reboot_count" << std::endl;
    my_logger_g.logger->info("[COOR]: fetching current_reboot_count!");
    // NOTE: need to take the prev. epoch as an idx, since epoch was increased
    // last iteration
    int current_reboot_count =
        helpers::chip_check_diagnostics(statistics_g.epochs);
    std::cout << "[COOR]: rbtcnt " << current_reboot_count << std::endl;
    bool spurrious_reboot = (current_reboot_count - reboot_count) !=
                            (fuzz_strategy_config_g.epoch_size);
    if (spurrious_reboot) {
      std::cout << "[COOR]: crash!" << std::endl;
      my_logger_g.logger->info("[COOR]: crash!");
      std::cout << "[COOR]: expected: " << fuzz_strategy_config_g.epoch_size
                << " but was " << current_reboot_count - reboot_count
                << std::endl;
      my_logger_g.logger->info("[COOR]: expected {} but was {}",
                               fuzz_strategy_config_g.epoch_size,
                               current_reboot_count - reboot_count);
      statistics_g.dut_crash_counter++;
    }
    reboot_count = current_reboot_count;

    /* remove the device from the matter network, just in case */
    // TODO: tidy this up!
    // NOTE: need to take the prev. epoch as an idx, since epoch was increased
    // last iteration
    std::cout << "unpairing node " << statistics_g.epochs << std::endl;
    int ret3 = std::system(
        (std::string("./connectedhomeip/out/chip-tool pairing unpair ") +
         std::to_string(statistics_g.epochs) + std::string(" | grep \"[TOO]\""))
            .c_str());
    if (ret3 == 124) {
      my_logger_g.logger->warn("Command \"{}\" timed out",
                               "chiptool clear session");
    } else if (ret3) {
      my_logger_g.logger->warn("Command \"{}\" failed with exit code: {}",
                               "chiptool clear session", ret3);
      std::cout << "[CHIP]: unpairing node has failed" << std::endl;
    }

    std::cout << "waiting some seconds to make sure mdns entries are evicted"
              << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(20));

    /* and pair the device again. */
    bool pstop = protocol_stack->stop();
    bool start = dut->start();
    std::this_thread::sleep_for(std::chrono::seconds(1));
    my_logger_g.logger->debug("WE ARE HERE NOW");
    bool reset = dut->factoryreset();
    bool pstart = protocol_stack->start();
    std::this_thread::sleep_for(std::chrono::seconds(10));
    if (helpers::chip_pair(statistics_g.epochs + 1) == 0) {
      reboot_count = helpers::chip_check_diagnostics(statistics_g.epochs + 1);
      statistics_g.dut_reboot_counter = reboot_count;
      std::cout << "AND HERE WE ARE DONE!" << std::endl;
      if (!(start && reset && pstart && pstop)) {
        my_logger_g.logger->error("scheduling a hard reset failed!");
        need_to_finish = true;
      }
    } else {
      need_to_finish = true;
    }

    // let fuzzing_loop continue fuzzing:
    is_epoch_it = false;
  }

  /* --------------- NOTE: from here no packets can flow! ------------------- */
  wdissector_mutex.lock();
  my_logger_g.logger->debug("[COOR]: RFI: entering lock");

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
    int need_to_finish_local = fuzzers[i]->prepare_new_iteration();
    need_to_finish |= !need_to_finish_local;
    if (need_to_finish_local == 0) {
      my_logger_g.logger->info("Fuzzer indexed {} requested finishing fuzzing",
                               i);
      print_statistics();
    }
  }

  need_to_finish |= (static_cast<int>(local_iteration) >=
                     (fuzz_strategy_config_g.total_iterations == -1
                          ? INT_MAX
                          : fuzz_strategy_config_g.total_iterations));

  wdissector_mutex.unlock();

  /* ----------------- NOTE: from here packets can flow! -------------------- */
  my_logger_g.logger->debug("[COOR]: RFI: exiting lock");

  /* Finish the fuzzing if needed */
  if (need_to_finish) {
    my_logger_g.logger->info("Finishing the fuzzing");
    return false;
  }

  // if (!dut->stop()) {
  //   my_logger_g.logger->error("DUT cannot be stopped");
  // } else {
  //   my_logger_g.logger->warn("DUT stopped successfully");
  // }
  // std::this_thread::sleep_for(std::chrono::seconds(2));

  /* Prepare for the new iteration */
  if (need_to_restart_protocol_stack || !protocol_stack->reset()) {
    my_logger_g.logger->error("Protocol stack cannot be restarted");
    return false;
  } else {
    my_logger_g.logger->warn("Protocol stack restarted successfully");
    my_logger_g.logger->warn(
        "Protocol stack reconfigured successfully after restart");
  }
  std::this_thread::sleep_for(std::chrono::seconds(2));
  if (!dut->reset()) {
    my_logger_g.logger->error("DUT cannot be restarted");
    return false;
  } else {
    my_logger_g.logger->warn("DUT restarted successfully");
  }

  need_to_restart_protocol_stack = false;
  stop_fuzzing_iteration = false;
  incoming_packet_num = 0;

  return true;
}

void Phys_Timeout_Based_Coordinator::layer_fuzzing_loop(EnumMutex mutex_num) {

  const std::string layer_name = helpers::get_layer_name_by_idx(mutex_num);
  my_logger_g.logger->info("Starting {} thread", layer_name);
  std::unique_ptr<SHM_Layer_Communication> SHM_Comm =
      SHM_Layer_Communication_Factory::
          get_shm_layer_communication_instance_by_layer_num(mutex_num);
  my_logger_g.logger->info("Inited communication");

  std::string dissector =
      helpers::get_dissector_by_layer_idx(static_cast<int>(mutex_num));

  while (SHM_Layer_Communication::is_active) {
    bool failed = false;

    /* Recieve intercepted message */
    Packet pdu = SHM_Comm->receive();

    if (!pdu.get_size())
      continue;

    pdu.set_dissector_name(dissector);

    wdissector_mutex.lock();
    my_logger_g.logger->debug("[COOR]: LFZ: entering lock");
    my_logger_g.logger->info("[{}] Dissector {} {}", layer_name, dissector,
                             pdu);

    if (pdu.get_packet_src() == PACKET_SRC::SRC_PROTOCOL_STACK) {
      if (!pdu.full_dissect()) {
        my_logger_g.logger->warn("[{}] Fuzz iteration failed in prepare_fuzz",
                                 layer_name);
        failed = true;
      }
      my_logger_g.logger->info("---> Dissector's summary: {}",
                               pdu.get_summary());
      my_logger_g.logger->flush();

      bool is_state_fuzzed = helpers::is_state_being_fuzzed(pdu.get_summary());

      if (!failed && is_state_fuzzed && !is_epoch_it) {
        for (size_t i = 0; i < fuzzers.size(); i++) {
          if (!fuzzers.at(i)->fuzz(pdu)) {
            my_logger_g.logger->warn("[{}] Fuzz iteration failed", layer_name);
            failed = true;
          }
        }

        std::string fuzzed_packet_type = "UNKNOWN";
        if (pdu.quick_dissect()) {
          fuzzed_packet_type = pdu.get_summary_short();
        }
        my_logger_g.logger->info("Fuzzed packet: {} (type {})", pdu,
                                 fuzzed_packet_type);
      }

      if (is_epoch_it) {
        my_logger_g.logger->info("[COOR]: not fuzzing, epoch it or post-epoch");
      } else {
        std::cout << "FUZZING" << std::endl;
      }
    } else {
      incoming_packet_num = incoming_packet_num + 1;
      if (pdu.quick_dissect()) {
        my_logger_g.logger->info("<--- Dissector's summary: {}",
                                 pdu.get_summary());
        if (!is_epoch_it) {
          advertisement_counter++;
        }
      } else {
        my_logger_g.logger->warn(
            "Failed to dissect the packet! (dissector: {})", dissector);
      }

      /* Check if we want to stop after the reception of the current message */
      const std::string &packet_summary = pdu.get_summary();
      const std::string &packet_summary_short = pdu.get_summary_short();
      const std::vector<std::string> &stop_after_state_vec =
          fuzz_strategy_config_g.fuzzing_stop_states;
      if (!stop_fuzzing_iteration &&
          (std::find(stop_after_state_vec.begin(), stop_after_state_vec.end(),
                     packet_summary) != stop_after_state_vec.end() ||
           std::find(stop_after_state_vec.begin(), stop_after_state_vec.end(),
                     packet_summary_short) != stop_after_state_vec.end())) {
        my_logger_g.logger->info("Message \"{}\" triggered termination of the "
                                 "current fuzzing iteration",
                                 packet_summary);
        stop_fuzzing_iteration = true;
        statistics_g.dut_become_router_counter++;
      }
    }
    my_logger_g.logger->info("");
    my_logger_g.logger->flush();

    if (pdu.get_packet_src() == PACKET_SRC::SRC_PROTOCOL_STACK) {
      Base_Fuzzer::packet_buffer[pdu.get_dissector_name()].insert(pdu);
    }
    wdissector_mutex.unlock();
    my_logger_g.logger->debug("[COOR]: LFZ: exiting lock");
    SHM_Comm->send(pdu);
    if (failed)
      statistics_g.has_this_iteration_failed = true;
  }
}

void Phys_Timeout_Based_Coordinator::fuzzing_loop() {
  // Dissector warm-up. For some reason, wdissector does not dissect the first
  // packet correctly.
  auto p = helpers::get_sample_packet();
  if (!p.quick_dissect()) {
    my_logger_g.logger->error("Dissector failed to dissect a sample packet!");
    return;
  }

  std::unique_ptr<std::thread> thread_mle;
  std::unique_ptr<std::thread> thread_coap;

  bool mle_thread_is_running = false;
  bool coap_thread_is_running = false;

  bool fuzz_MLE = fuzz_config_g.FUZZ_MLE;
  bool fuzz_COAP = fuzz_config_g.FUZZ_COAP;

  if (fuzz_MLE) {
    mle_thread_is_running = true;
    thread_mle = std::make_unique<std::thread>(
        &Phys_Timeout_Based_Coordinator::layer_fuzzing_loop, this,
        SHM_MUTEX_MLE);
  }

  if (fuzz_COAP) {
    coap_thread_is_running = true;
    thread_coap = std::make_unique<std::thread>(
        &Phys_Timeout_Based_Coordinator::layer_fuzzing_loop, this,
        SHM_MUTEX_COAP);
  }

  std::cout << "BLUG: CALLING RESTART ON NODES" << std::endl;

  std::cout << "BLUG: RESTARTING BR" << std::endl;

  if (!protocol_stack->restart()) {
    std::cout << "Failed to start a protocol stack" << std::endl;
    my_logger_g.logger->error("Failed to start a protocol stack");
    Base_Coordinator::terminate_fuzzing();
    goto exit;
  }

  std::cout << "BLUG: RESTARTING DUT" << std::endl;

  if (!dut->restart()) {
    std::cout << "Failed to start a DUT" << std::endl;
    my_logger_g.logger->error("Failed to start a DUT");
    Base_Coordinator::terminate_fuzzing();
    goto exit;
  }

  std::cout << "restarts successfull" << std::endl;
  my_logger_g.logger->info("restarts succesfull");

  thread_dut_communication_func();

exit:
  std::cout << "killing everything" << std::endl;
  my_logger_g.logger->error("killing everything");

  if (mle_thread_is_running) {
    thread_mle->join();
  }

  if (coap_thread_is_running) {
    thread_coap->join();
  }
}

bool Phys_Timeout_Based_Coordinator::reset_target() { return true; }

std::string Phys_Timeout_Based_Coordinator::get_name() { return name_; }
