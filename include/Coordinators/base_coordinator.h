#pragma once

#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

#include "coverage_tracker.h"
#include "Fuzzers/base_fuzzer.h"
#include "DUT/DUT_base.h"
#include "Protocol_Stack/protocol_stack_base.h"

#include "shm/shared_memory.h"

class Base_Coordinator {
public:

    Base_Coordinator();
    virtual ~Base_Coordinator();

    // Interface functions
    virtual bool init_fuzzing_strategies(const std::vector<std::string>& fuzz_strategy_config_names);
    virtual bool init(const std::vector<std::string>& fuzz_strategy_config_names) = 0;
    virtual void deinit() = 0;
    virtual void thread_dut_communication_func() = 0;
    virtual bool reset_target() = 0;

    void fuzzing_loop();

    virtual bool renew_fuzzing_iteration();
    virtual void print_statistics();

    std::vector<std::unique_ptr<Base_Fuzzer>> fuzzers;

    virtual std::string get_name() {
        return "";
    }

    void update_coverage_information();
    void update_probabilities(bool found_new_coverage);

    double calculate_field_mutation_score();
    double coverage_formula_1(double beta);

protected:
    std::unique_ptr<Protocol_Stack_Base> protocol_stack;
    std::unique_ptr<DUT_Base> dut;

    std::mutex wdissector_mutex;
    std::vector<std::unique_ptr<Coverage_Tracker>> coverage_trackers;
    uint32_t local_iteration;
    uint32_t global_iteration = 0;
    std::ofstream statistics;

    std::ofstream save_all_seeds_to;
    std::string save_all_seeds_to_filename = "seeds/all_seeds.json";
    std::string hang_seeds_dir = "seeds/hang_seeds/";
    std::string crash_seeds_dir = "seeds/crash_seeds/";
    std::string statistics_file_name;

    std::vector<Fuzz_Strategy_Config> fuzz_strategy_configs_;

    void layer_fuzzing_loop(EnumMutex mutex_num); // Mutex number
    void terminate_fuzzing();
    
    bool prepare_new_fuzzing_sprint(const Fuzz_Strategy_Config& fuzz_strategy_config);

    struct IterationResult {
        bool was_new_coverage_found;
        size_t new_coverage_found;
        bool was_crash_detected;
    } iteration_result;

    volatile bool stop_fuzzing_iteration = false;
  volatile int incoming_packet_num = 0;
};

//std::unique_ptr<Base_Coordinator> get_coordinator_by_target(TARGET t);
// std::unique_ptr<Protocol_Stack> get_protocol_stack_by_name(PROTOCOL_STACK_NAME s);
