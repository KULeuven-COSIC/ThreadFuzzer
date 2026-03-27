#pragma once

#include "Coordinators/base_coordinator.h"
#include "shared_memory.h"

#include <string>

class Phys_Timeout_Based_Coordinator final : public Base_Coordinator {
public:
    Phys_Timeout_Based_Coordinator();
    bool init(const std::vector<std::string>& fuzz_strategy_config_names) override;
    void deinit() override;
    void thread_dut_communication_func() override;
    bool reset_target() override;
    bool renew_fuzzing_iteration() override;

    void fuzzing_loop() override;

    void layer_fuzzing_loop(EnumMutex mutex_num) override;
    std::string get_name() override;
private:
    std::string name_ = "Physical Timeout-Based Coordinator";
    bool need_to_restart_protocol_stack = false;
    bool need_to_restart_dut = false;
    int reboot_count = 0;
    bool is_epoch_it = true; // initialized at true, making sure initial steps aren't fuzzed
};
