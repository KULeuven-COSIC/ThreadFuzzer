#include "DUT/alexa.h"

#include "my_logger.h"
#include "Configs/Fuzzing_Settings/technical_config.h"
#include "Configs/Fuzzing_Settings/timers_config.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

extern My_Logger my_logger_g;
extern Technical_Config technical_config_g;
extern Timers_Config timers_config_g;

Alexa::Alexa() {
    // std::string command = "python3 scripts/tapo_plug_session.py -p " + technical_config_g.tapo_pipe_name;
    // if (std::system(command.c_str()) != 0) {
    //     my_logger_g.logger->error("Failed to initialize a tapo plug session");
    //     throw std::runtime_error("Failed to initialize a tapo plug session");
    // }
}

Alexa::~Alexa() {
    echo_to_pipe("exit");
}

bool Alexa::start() {
    power_on();
    return true;
}

bool Alexa::stop() {
    power_off();
    return true;
}

bool Alexa::restart() {
    restart(timers_config_g.tapo_restart_wait_time_s);
    return true;
}

bool Alexa::is_running() {
    return true;
}

bool Alexa::reset()
{
    return restart();
}

void Alexa::power_on() {
    echo_to_pipe("on");
}

void Alexa::power_off() {
    echo_to_pipe("off");
}

void Alexa::restart(int wait_time_s) {
    power_off();
    std::this_thread::sleep_for(std::chrono::seconds(wait_time_s));
    power_on();
}

bool Alexa::echo_to_pipe(const std::string cmd) {
    if (!std::filesystem::exists(technical_config_g.tapo_pipe_name)) {
        my_logger_g.logger->error("The pipe {} does not exist", technical_config_g.tapo_pipe_name);
        return false;
    }
    std::ofstream pipe;
    pipe.open(technical_config_g.tapo_pipe_name);
    if (pipe.is_open()) {
        pipe << cmd << "\n";
    } else {
        my_logger_g.logger->warn("Failed to open pipe: {}", technical_config_g.tapo_pipe_name);
        return false;
    }
    return true;
}

/**
   TODO: stub!!
 */
bool Alexa::factoryreset() { return true; }
