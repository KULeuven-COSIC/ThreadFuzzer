#pragma once

#include "Coordinators/base_coordinator.h"
#include "shared_memory.h"

#include <string>

class Timeout_Based_Coordinator final : public Base_Coordinator {
public:
    Timeout_Based_Coordinator();
    bool init(const std::vector<std::string>& fuzz_strategy_config_names) override;
    void deinit() override;
    void thread_dut_communication_func() override;
    bool reset_target() override;
    bool renew_fuzzing_iteration() override;

    void layer_fuzzing_loop(EnumMutex enum_num) override;

    void fuzzing_loop() override;
    std::string get_name() override;

  private:
    std::string name_ = "Timeout-Based Coordinator";
};
