#include "Coordinators/active_coordinator.h"

#include "Configs/Fuzzing_Settings/main_config.h"
#include "Configs/Fuzzing_Strategies/fuzz_strategy_config.h"
#include "Communication/shm_layer_communication.h"

#include "coverage_tracker.h"
#include "my_logger.h"
#include "statistics.h"

#include <algorithm>
#include <string>
#include <vector>

extern Main_Config main_config_g;
extern Fuzz_Strategy_Config fuzz_strategy_config_g;
extern My_Logger my_logger_g;
extern std::string dut_log_file_name_g;
extern Statistics statistics_g;

Active_Coordinator::Active_Coordinator()
{
        // if (fuzz_strategy_config_g.use_coverage_logging || fuzz_strategy_config_g.use_coverage_feedback) coverage_trackers.emplace_back(std::make_unique<Coverage_Tracker>("ENB_COVERAGE_TRACKER", "tcp://127.0.0.1:5567"));
        // if (fuzz_strategy_config_g.use_coverage_logging || fuzz_strategy_config_g.use_coverage_feedback) coverage_trackers.emplace_back(std::make_unique<Coverage_Tracker>("EPC_COVERAGE_TRACKER", "tcp://127.0.0.1:5568"));
}

Active_Coordinator::~Active_Coordinator() {}

bool Active_Coordinator::init(const std::vector<std::string>& fuzz_strategy_config_names) {
    if (!init_fuzzing_strategies(fuzz_strategy_config_names)) {
        my_logger_g.logger->error("Failed to init the fuzzing strategies");
        return false;
    }
    return true;
}

void Active_Coordinator::deinit() {
    return;
}

void Active_Coordinator::thread_dut_communication_func() {

    for (const Fuzz_Strategy_Config& fuzz_strategy_config : fuzz_strategy_configs_) {

        if (!prepare_new_fuzzing_sprint(fuzz_strategy_config)) {
            my_logger_g.logger->error("Failed to prepare new fuzzing sprint");
            break;
        }

        while(SHM_Layer_Communication::is_active) {
            
            my_logger_g.logger->info("================ START OF A NEW FUZZING ITERATION {} ================", global_iteration);
            my_logger_g.logger->flush();

            try {

                if (!protocol_stack->is_running()) {
                    my_logger_g.logger->warn("Protocol stack is not running. Restarting...");
                    if (!protocol_stack->restart()) {
                        my_logger_g.logger->error("Protocol stack cannot be restarted");
                        Base_Coordinator::terminate_fuzzing();
                        return;
                    } else {
                        my_logger_g.logger->info("Protocol stack restarted successfully");
                    }
                }

                if (!protocol_stack->reset()) {
                    my_logger_g.logger->error("Failed to reset a protocol stack");
                    Base_Coordinator::terminate_fuzzing();
                    return;
                }

                if (!renew_fuzzing_iteration()) {
                    break;
                }
            } catch(std::runtime_error& ex) {
                my_logger_g.logger->warn("Exception happened: {}", ex.what());
                if (std::string(ex.what()) == std::string(MODEM_DEAD)) {
                    statistics_g.ue_crash_counter++;
                    if (ue_->restart()) {
                        my_logger_g.logger->info("Restarted the DUT successfully");
                    } 
                    else {
                        terminate_fuzzing();
                        return;
                    }
                } else {
                    terminate_fuzzing();
                    return;
                }
            }
        }
        if (!SHM_Layer_Communication::is_active) break;
    }
    terminate_fuzzing();
}

bool Active_Coordinator::reset_target()
{
    wdissector_mutex.unlock();
    for (int cur_try = 0; ; cur_try++) {
        //if (dut_comm->disconnect_from_network()) break;
        if (ue_->disconnect_from_network()) break;
        if (cur_try == max_tries) break;//throw std::runtime_error("DUT COULDN'T DISCONNECT FROM THE NETWORK");
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    return true;
}

std::string Active_Coordinator::get_name() {
    return name_;
}