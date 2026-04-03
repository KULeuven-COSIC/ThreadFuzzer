#include "Coordinators/phys_timeout_based_coordinator.h"

#include "Communication/shm_layer_communication.h"
#include "Configs/Fuzzing_Settings/timers_config.h"
#include "Communication/shm_layer_communication_factory.h"
#include "Communication/shm_layer_communication.h"
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
}

bool Phys_Timeout_Based_Coordinator::init(
    const std::vector<std::string> &fuzz_strategy_config_names) {
  if (!init_fuzzing_strategies(fuzz_strategy_config_names)) {
    my_logger_g.logger->error("Failed to init the fuzzing strategies");
    return false;
  }

  return true;
}

void Phys_Timeout_Based_Coordinator::deinit() {
  return;
}

void Phys_Timeout_Based_Coordinator::thread_dut_communication_func() {
    for (const Fuzz_Strategy_Config &fuzz_strategy_config : fuzz_strategy_configs_) {

        if (!prepare_new_fuzzing_sprint(fuzz_strategy_config)) {
            my_logger_g.logger->error("Failed to prepare new fuzzing sprint");
            break;
        }

        while (SHM_Layer_Communication::is_active) {
            
            // --- 1. INITIAL COMMISSIONING (First Iteration Only) ---
            if (local_iteration == 0 && fuzz_strategy_config_g.chip_recommissioning_step) {
                my_logger_g.logger->info("[COOR]: Initializing pairing for new sprint (Node ID: {})", node_id);
                
                if (!helpers::chip_pair(node_id, main_config_g.chip_passcode, main_config_g.chip_discriminator)) {
                    my_logger_g.logger->error("[COOR]: Initial pairing failed. Terminating sprint.");
                    /* TODO: ADD RECOVERY MECHANISM */
                    terminate_fuzzing();
                    break;
                }
                try {
                    reboot_count = helpers::chip_fetch_reboot_count(node_id);
                } catch(std::exception& ex) {
                    /* TODO: ADD RECOVERY MECHANISM */
                    terminate_fuzzing();
                }
                statistics_g.dut_reboot_counter = reboot_count;
                my_logger_g.logger->info("[COOR]: Baseline reboot count established: {}", reboot_count);

                clean_iteration.store(false);

                if (!protocol_stack->reset()) {
                    my_logger_g.logger->error("[COOR]: Failed to restart protocol stack post-pairing. Exiting.");
                    terminate_fuzzing();
                    break;
                }
                
                dut->reset();
            }

            // --- 2. ACTIVE FUZZING / MONITORING LOOP ---
            my_logger_g.logger->info("================ START OF A NEW FUZZING ITERATION {} ================", global_iteration);
            my_logger_g.logger->flush();

            int counter = timers_config_g.iteration_length_s;
            int current_silent_time = 0;
            int old_incoming_packet_num = 0;
            int iteration_time = 0;

            while (SHM_Layer_Communication::is_active && !stop_fuzzing_iteration && counter > 0) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
                
                iteration_time++;
                counter--;

                if (!protocol_stack->is_running()) {
                    need_to_restart_protocol_stack = true;
                    my_logger_g.logger->warn("Protocol stack stopped running. Aborting current iteration.");
                    break;
                }

                // Evaluate DUT responsiveness
                if (incoming_packet_num == old_incoming_packet_num) {
                    current_silent_time++;

                    if (current_silent_time >= timers_config_g.empty_iteration_length_s) {
                        // Distinguish between a completely dead iteration vs. crashing mid-iteration
                        if (incoming_packet_num == 0) {
                            statistics_g.empty_iterations++;
                            my_logger_g.logger->warn("Iteration completely empty (no incoming packets). Ending early.");
                        } else {
                            statistics_g.long_silence++;
                            my_logger_g.logger->warn("DUT fell silent (potential crash). Ending iteration early.");
                        }
                        break; // Break out of the iteration waiting loop
                    }
                } else {
                    // DUT is actively sending packets; reset the silence timer
                    current_silent_time = 0;
                    old_incoming_packet_num = incoming_packet_num;
                }
            }

            // --- 3. POST-ITERATION STATS & RENEWAL ---
            my_logger_g.logger->info("Iteration {} finished. Duration: {} seconds.", global_iteration, iteration_time);
            
            // Cumulative moving average of iteration time
            statistics_g.avg_iteration_time_s += 
                (static_cast<double>(iteration_time) - statistics_g.avg_iteration_time_s) / (global_iteration + 1);

            if (!SHM_Layer_Communication::is_active) {
                break;
            }

            // Delegate the heavy lifting of preparing the next iteration to our refactored function
            if (!renew_fuzzing_iteration()) {
                my_logger_g.logger->info("Fuzzing renewal requested termination.");
                break;
            }
        }
        
        // Break out of the outer fuzz strategy loop if the shared memory layer dies
        if (!SHM_Layer_Communication::is_active) {
            break;
        }
    }
    
    terminate_fuzzing();
}

bool Phys_Timeout_Based_Coordinator::renew_fuzzing_iteration() {
    my_logger_g.logger->info("Renewing fuzzing iteration");
    bool need_to_finish = false;

    // --- 1. STATE MANAGEMENT ---
    // Capture the exact state of the iteration that JUST finished 
    const bool finished_iteration_was_clean{clean_iteration.load()};

    global_iteration++;
    local_iteration++;

    if (local_iteration % fuzz_strategy_config_g.epoch_size == fuzz_strategy_config_g.epoch_size - 1) {
        my_logger_g.logger->info("[COOR]: Preparing for CLEAN Epoch Iteration");
        statistics_g.epochs++;
        statistics_g.it_in_epochs = 0;
        clean_iteration.store(true);
    } else {
        statistics_g.it_in_epochs++;
    }

    // --- 2. EPOCH VALIDATION & DEVICE RE-PAIRING ---
    // If modulo is 0, we just finished a full epoch (meaning 'finished_iteration' WAS the clean one).
    if (local_iteration > 0 && local_iteration % fuzz_strategy_config_g.epoch_size == 0) {
        
        if (!finished_iteration_was_clean) {
            my_logger_g.logger->error("[COOR]: State Machine Error! Expected to be exiting a clean iteration.");
            need_to_finish = true;
        }

        my_logger_g.logger->info("[COOR]: Fetching current_reboot_count to check for crashes...");
        try {
            int current_reboot_count = helpers::chip_fetch_reboot_count(node_id);
            bool spurrious_reboot = (current_reboot_count - reboot_count) != fuzz_strategy_config_g.epoch_size;
            if (spurrious_reboot) {
                my_logger_g.logger->warn("[COOR]: CRASH DETECTED! Expected {} reboots but saw {}",
                                        fuzz_strategy_config_g.epoch_size,
                                        current_reboot_count - reboot_count);
                statistics_g.dut_crash_counter++;
            }
            reboot_count = current_reboot_count;
        } catch (std::exception& ex) {
            my_logger_g.logger->debug("Exception during the reboot count fetch: {}", ex.what());
            my_logger_g.logger->warn("Failed to read a reboot count. Ignoring it during this iteration.");
            statistics_g.broken_iterations++;
            }

        // Unpair the device
        if (!helpers::chip_unpair(node_id)) {
            my_logger_g.logger->warn("Chip unpair failed for Node ID: {}", node_id);
            my_logger_g.logger->debug("Still continuing hoping to recover.");
        }

        // Wait for host mDNS caches to gracefully expire after a potential dirty crash
        my_logger_g.logger->debug("Waiting 20 seconds for mDNS cache eviction...");
        std::this_thread::sleep_for(std::chrono::seconds(20));

        // Restart stack and device
        bool restart_success = restart_stack_and_device();

        // Wait for the Border Router to form the Thread partition and initialize radios
        my_logger_g.logger->debug("Waiting 10 seconds for Protocol Stack network initialization...");
        std::this_thread::sleep_for(std::chrono::seconds(10));

        bool done = false;
        int counter = 5;
        while (!done && --counter) {
            // Re-pair the device
            if (helpers::chip_pair(node_id, main_config_g.chip_passcode, main_config_g.chip_discriminator)) {
                try {
                    reboot_count = helpers::chip_fetch_reboot_count(node_id);
                } catch (std::exception& ex) {
                    /* REBOOT COUNT FETCH FAILED. RECOVERING... */
                    my_logger_g.logger->debug("Trying to recover from crash");
                    my_logger_g.logger->debug("Protocol stack running: {}", protocol_stack->is_running());
                    restart_success = restart_stack_and_device();
                    my_logger_g.logger->debug("Recovered from crash");
                    my_logger_g.logger->debug("Waiting 10 seconds for Protocol Stack network initialization...");
                    std::this_thread::sleep_for(std::chrono::seconds(10));
                    statistics_g.broken_pairings++;
                    continue;
                }
                done = true; /* The only success */
                statistics_g.dut_reboot_counter = reboot_count;
            } else {
                /* PAIRING FAILED. RECOVERING... */
                my_logger_g.logger->error("Failed to re-pair CHIP device after epoch reset.");
                my_logger_g.logger->debug("Protocol stack running: {}", protocol_stack->is_running());
                my_logger_g.logger->debug("Trying to recover from crash");
                restart_success = restart_stack_and_device();
                my_logger_g.logger->debug("Recovered from crash");
                my_logger_g.logger->debug("Waiting 10 seconds for Protocol Stack network initialization...");
                std::this_thread::sleep_for(std::chrono::seconds(10));
            }
        }

        if (!restart_success) {
            my_logger_g.logger->error("Scheduling a hard reset failed during epoch setup!");
            need_to_finish = true;
        }

        if (counter == 0) {
            need_to_finish = true; // Failed to recover from crash. Terminating
            return false;
        }

        clean_iteration.store(false); // Back to fuzzing
    }


    // --- 3. COVERAGE & PROBABILITY UPDATES ---
    wdissector_mutex.lock();
    my_logger_g.logger->debug("[COOR]: RFI: entering lock");

    if (!finished_iteration_was_clean) {
      try {
          // BUG FIX: We now use the cleanly preserved boolean to completely bypass
          // the coverage logic if the iteration that just ran was the clean one.
          if (fuzz_strategy_config_g.use_coverage_logging || fuzz_strategy_config_g.use_coverage_feedback) {
              update_coverage_information();
              if (fuzz_strategy_config_g.use_coverage_feedback) {
                  update_probabilities(iteration_result.was_new_coverage_found);
              }
          }
      } catch (const std::exception &ex) {
          my_logger_g.logger->warn("Exception during the coverage fetch: {}", ex.what());
          if (!protocol_stack->is_running()) {
              need_to_restart_protocol_stack = true;
          }
      }

      Base_Fuzzer::mut_field_num_tracker.push_mutated_field_num(Base_Fuzzer::mutated_fields_num);

      if (Base_Fuzzer::mutated_fields_num == 0) {
          statistics_g.empty_iterations++;
      }

      my_logger_g.logger->debug("Number of mutated fields in this iteration: {}", Base_Fuzzer::mutated_fields_num);
      
      Base_Fuzzer::mutated_fields.clear();
      Base_Fuzzer::mutated_fields_num = 0;

      if (fuzz_strategy_config_g.use_probability_resets && Base_Fuzzer::mut_field_num_tracker.needs_reset()) {
          Base_Fuzzer::mut_field_num_tracker.reset();
          my_logger_g.logger->info("Resetting the probabilities");
          Base_Fuzzer::packet_field_tree_database.clear();
      }
    }

    // --- 4. ERROR HANDLING & FUZZER POLLING ---
    if (need_to_restart_protocol_stack) {
        statistics_g.protocol_stack_crash_counter++;
        my_logger_g.logger->warn("Protocol Stack has crashed!");
        
        if (!main_config_g.gen_log_file.empty()) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            std::string crash_info = helpers::read_file_last_chars(main_config_g.gen_log_file);
            my_logger_g.logger->info("Protocol Stack output:\n {}", crash_info);
        }
    }

    print_statistics();

    // Check if any of the fuzzers request to finish
    for (size_t i = 0; i < fuzzers.size(); i++) {
        int need_to_finish_local = fuzzers[i]->prepare_new_iteration();
        need_to_finish |= !need_to_finish_local;
        
        if (need_to_finish_local == 0) {
            my_logger_g.logger->info("Fuzzer indexed {} requested finishing fuzzing", i);
            print_statistics();
        }
    }

    need_to_finish |= (static_cast<int>(local_iteration) >=
                       (fuzz_strategy_config_g.total_iterations == -1 ? INT_MAX : fuzz_strategy_config_g.total_iterations));

    wdissector_mutex.unlock();
    my_logger_g.logger->debug("[COOR]: RFI: exiting lock");


    // --- 5. ITERATION TEARDOWN / RESTART ---
    if (need_to_finish) {
        my_logger_g.logger->info("Finishing the fuzzing");
        return false;
    }

    if (need_to_restart_protocol_stack || !protocol_stack->reset()) {
        my_logger_g.logger->error("Protocol stack cannot be restarted");
        return false;
    } else {
        my_logger_g.logger->info("Protocol stack reconfigured and restarted successfully");
    }

    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    if (!dut->reset()) {
        my_logger_g.logger->error("DUT cannot be restarted");
        return false;
    } else {
        my_logger_g.logger->debug("DUT restarted successfully");
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

      if (!failed && is_state_fuzzed && !clean_iteration.load()) {
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

      if (clean_iteration.load()) {
        my_logger_g.logger->info("[COOR]: not fuzzing, epoch it or post-epoch");
      } else {
        std::cout << "FUZZING" << std::endl;
      }
    } else {
      incoming_packet_num = incoming_packet_num + 1;
      if (pdu.quick_dissect()) {
        my_logger_g.logger->info("<--- Dissector's summary: {}",
                                 pdu.get_summary());
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

bool Phys_Timeout_Based_Coordinator::restart_stack_and_device() {
    // Restart stack and device
    bool pstop = protocol_stack->stop();
    bool start = dut->start();
    std::this_thread::sleep_for(std::chrono::seconds(1));
    bool reset = dut->factoryreset();
    bool pstart = protocol_stack->start();
    return pstop && start && reset && pstart;
}

bool Phys_Timeout_Based_Coordinator::reset_target() { return true; }

std::string Phys_Timeout_Based_Coordinator::get_name() { return name_; }
