#include "Protocol_Stack/openthread_stack.h"

#include "Protocol_Stack/Packet_Generator/OT_packet_generator_factory.h"

#include "helpers.h"
#include "my_logger.h"
#include "statistics.h"
#include "Configs/Fuzzing_Settings/main_config.h"
#include "Configs/Fuzzing_Settings/technical_config.h"
#include "Configs/Fuzzing_Settings/timers_config.h"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>

extern Main_Config main_config_g;
extern My_Logger my_logger_g;
extern Technical_Config technical_config_g;
extern Timers_Config timers_config_g;
extern Statistics statistics_g;

OpenThread_Stack::OpenThread_Stack() {
    packet_generator_ = OT_Packet_Generator_Factory::get_OT_packet_generator_by_name(main_config_g.packet_generator_name);
}

bool OpenThread_Stack::start()
{
    if (!packet_generator_->reset()) {
        std::cerr << "Failed to start OT protocol stack HELP!!" << std::endl;
        my_logger_g.logger->error("Failed to start OT protocol stack");
        return false;
    }

    std::this_thread::sleep_for(std::chrono::seconds(2));

    /* Check if everything is running fine */
    if (!packet_generator_->is_running()) {
        my_logger_g.logger->error("OT protocol stack is not alive");
        return false;
    }

    return true;
}

bool OpenThread_Stack::stop()
{
    bool success = true;

    if (!packet_generator_->stop()) {
        my_logger_g.logger->warn("Failed to stop OT protocol stack");
        success = false;
    }

    return success;
}

bool OpenThread_Stack::restart()
{
  // my_logger_g.logger->debug("Stopping the OT protocol stack.");
  // stop();
  // std::this_thread::sleep_for(std::chrono::seconds(timers_config_g.protocol_stack_restart_timer_s));
  // my_logger_g.logger->debug("Starting the OT protocol stack.");
  // if (!start()) return false;
  // return true;
  return packet_generator_->restart();
}

bool OpenThread_Stack::is_running()
{
    bool packet_generator_alive = packet_generator_->is_running();
    return packet_generator_alive;
}

bool OpenThread_Stack::reset() {
    return packet_generator_->reset();
}

