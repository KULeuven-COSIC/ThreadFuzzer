#pragma once

#include <string>
#include <vector>

#include <nlohmann/json.hpp>

class Timers_Config {
public:
    int iteration_length_s = 60;
    int iteration_length_ms = 1000;
    int router_selection_jitter_s = 10;
    int protocol_stack_restart_timer_s = 1;
    int dut_restart_timer_s = 1;

    int system_cmd_max_timeout_default_s = 10;
    int tapo_restart_wait_time_s = 5;
    int speed = 1;
    int node_async_reader_timeout_ms = 100;

    int empty_iteration_length_s = 5;
    int empty_iteration_length_ms = 50;

    int coverage_request_max_timeout_s = 3;

    std::ostream& dump (std::ostream& os) const;
};

std::ostream& operator<< (std::ostream& os, const Timers_Config& config);

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
    Timers_Config, protocol_stack_restart_timer_s, dut_restart_timer_s,
    iteration_length_ms, iteration_length_s, router_selection_jitter_s,
    system_cmd_max_timeout_default_s, tapo_restart_wait_time_s, speed,
    empty_iteration_length_ms, empty_iteration_length_s, node_async_reader_timeout_ms,
    coverage_request_max_timeout_s)
