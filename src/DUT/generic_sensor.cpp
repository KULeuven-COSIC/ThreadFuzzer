#include "DUT/generic_sensor.h"

#include "Configs/Fuzzing_Settings/technical_config.h"
#include "Configs/Fuzzing_Settings/timers_config.h"
#include "helpers.h"
#include "my_logger.h"

#include <chrono>
#include <filesystem>
#include <iostream>
#include <ostream>
#include <thread>

extern My_Logger my_logger_g;
extern Technical_Config technical_config_g;
extern Timers_Config timers_config_g;

Generic_Sensor::Generic_Sensor(const std::string& name, int fr_duration_s) : name_(name), fr_duration_s_(fr_duration_s) {}

bool Generic_Sensor::start() {
    my_logger_g.logger->debug("[{}]:  STARTING", name_);
    if (!helpers::send_command_to_device(technical_config_g.dut_pipe_name, "ON")) {
        my_logger_g.logger->error("[{}]:  Failed to send ON command.", name_);
        return false;
    }
    my_logger_g.logger->info("[{}]:  Started successfully.", name_);
    
    constexpr int STABILITY_DELAY_SECONDS = 10;
    my_logger_g.logger->debug("[{}]:  Waiting {} seconds for stability...", name_, STABILITY_DELAY_SECONDS);
    std::this_thread::sleep_for(std::chrono::seconds(STABILITY_DELAY_SECONDS));
    
    my_logger_g.logger->debug("[{}]:  Wait complete. Sensor is stable.", name_);
    return true;
}

bool Generic_Sensor::stop() {
    my_logger_g.logger->debug("[{}]:  Attempting to STOP", name_);
    bool success = helpers::send_command_to_device(technical_config_g.dut_pipe_name, "OFF");
    if (!success) {
        my_logger_g.logger->error("[{}]:  Failed to stop.", name_);
    } else {
        my_logger_g.logger->info("[{}]:  Stopped successfully.", name_);
    }
    return success;
}

bool Generic_Sensor::restart() {
    my_logger_g.logger->info("[{}]:  Initiating RESTART sequence.", name_);
    if (!stop()) {
        my_logger_g.logger->error("[{}]:  Restart failed during STOP phase.", name_);
        return false;
    }
    constexpr int POWER_OFF_DURATION_SECONDS = 3;
    my_logger_g.logger->debug("[{}]:  Waiting {} seconds for power off...", name_, POWER_OFF_DURATION_SECONDS);
    std::this_thread::sleep_for(std::chrono::seconds(POWER_OFF_DURATION_SECONDS));
    if (!start()) {
        my_logger_g.logger->error("[{}]:  Restart failed during START phase.", name_);
        return false;
    }
    my_logger_g.logger->debug("[{}]:  Performing factory reset...", name_);
    if (!factoryreset()) {
        my_logger_g.logger->error("[{}]:  Restart failed during FACTORY RESET phase.", name_);
        return false;
    }
    my_logger_g.logger->info("[{}]:  RESTART SUCCESSFUL.", name_);
    return true;
}

bool Generic_Sensor::is_running() { 
    // Always return true
    return true;
}

bool Generic_Sensor::reset() {
    my_logger_g.logger->info("[{}]:  Initiating RESET sequence.", name_);
    if (!stop()) {
        my_logger_g.logger->error("[{}]:  Reset failed during STOP phase.", name_);
        return false;
    }
    constexpr int POWER_OFF_DURATION_SECONDS = 3;
    my_logger_g.logger->debug("[{}]:  Waiting {} seconds for power off...", name_, POWER_OFF_DURATION_SECONDS);
    std::this_thread::sleep_for(std::chrono::seconds(POWER_OFF_DURATION_SECONDS));
    if (!start()) {
        my_logger_g.logger->error("[{}]:  Reset failed during START phase.", name_);
        return false;
    }
    my_logger_g.logger->info("[{}]:  RESET SUCCESSFUL.", name_);
    return true;
}

bool Generic_Sensor::factoryreset() {
    my_logger_g.logger->info("[{}]:  Initiating Factory Reset...", name_);
    
    if (!helpers::send_command_to_device(technical_config_g.dut_pipe_name, "FR ON")) {
        my_logger_g.logger->error("[{}]:  Factory Reset failed to start ('FR ON' rejected).", name_);
        return false;
    }
    my_logger_g.logger->debug("[{}]:  'FR ON' sent. Waiting {} seconds...", name_, fr_duration_s_);
    std::this_thread::sleep_for(std::chrono::seconds(fr_duration_s_));
    if (!helpers::send_command_to_device(technical_config_g.dut_pipe_name, "FR OFF")) {
        my_logger_g.logger->error("[{}]:  Factory Reset failed to stop ('FR OFF' rejected).", name_);
        return false;
    }
    constexpr int SETTLE_DURATION_SECONDS = 5;
    my_logger_g.logger->debug("[{}]:  'FR OFF' sent. Allowing {} seconds to settle...", name_, SETTLE_DURATION_SECONDS);
    std::this_thread::sleep_for(std::chrono::seconds(SETTLE_DURATION_SECONDS));
    
    my_logger_g.logger->info("[{}]:  Factory Reset complete!", name_);
    return true;
}