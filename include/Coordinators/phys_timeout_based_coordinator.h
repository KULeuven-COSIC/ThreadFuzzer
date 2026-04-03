#pragma once

#include "Coordinators/base_coordinator.h"

#include <atomic>
#include <string>

class Phys_Timeout_Based_Coordinator final : public Base_Coordinator {
public:
    Phys_Timeout_Based_Coordinator();
    bool init(const std::vector<std::string>& fuzz_strategy_config_names) override;
    void deinit() override;
    void thread_dut_communication_func() override;
    bool reset_target() override;
    bool renew_fuzzing_iteration() override;

    std::string get_name() override;
private:
    void layer_fuzzing_loop(EnumMutex mutex_num) override; 

    std::string name_ = "Physical Timeout-Based Coordinator";
    bool need_to_restart_protocol_stack = false;

    std::atomic<bool> clean_iteration{true}; // Flag. If true, the next iteration is used to fetch the reboot count and therefore will not be fuzzed.
    int reboot_count = -1;

    const int node_id = 6; // TODO: Remove this
};
