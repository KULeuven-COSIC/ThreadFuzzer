#include "DUT/nanoleaf.h"

#include "my_logger.h"
#include "Configs/Fuzzing_Settings/technical_config.h"
#include "Configs/Fuzzing_Settings/timers_config.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>

extern My_Logger my_logger_g;
extern Technical_Config technical_config_g;
extern Timers_Config timers_config_g;

Nanoleaf::Nanoleaf() {
    // std::string command = "python3 scripts/tapo_plug_session.py -p " + technical_config_g.tapo_pipe_name;
    // if (std::system(command.c_str()) != 0) {
    //     my_logger_g.logger->error("Failed to initialize a tapo plug session");
    //     throw std::runtime_error("Failed to initialize a tapo plug session");
    // }
}

Nanoleaf::~Nanoleaf() {
    echo_to_pipe("exit");
}

bool Nanoleaf::start() {
    power_on();
    return true;
}

bool Nanoleaf::stop() {
    power_off();
    return true;
}

bool Nanoleaf::restart() {
    restart(timers_config_g.tapo_restart_wait_time_s);
    return true;
}

bool Nanoleaf::is_running() {
    return true;
}

// called after each iteration... (no touch-e!!)
bool Nanoleaf::reset()
{
    return restart();
}

// only called at the start (normally)
bool Nanoleaf::factoryreset()
{
    std::cout << "[NANOLEAF]: FACTORYRESET" << std::endl;
    std::cout << "going: ";
    for (int i = 0; i < 5; i++) {
      restart(1);
      std::cout << i << " ";
    }
    std::cout << std::endl;
    return true;
}

void Nanoleaf::power_on() {
    echo_to_pipe("on");
}

void Nanoleaf::power_off() {
    echo_to_pipe("off");
}

void Nanoleaf::restart(int wait_time_s) {
    power_off();
    std::this_thread::sleep_for(std::chrono::seconds(wait_time_s));
    std::cout << "WAITED fOR " << wait_time_s << std::endl;
    power_on();
}

bool Nanoleaf::echo_to_pipe(const std::string cmd) {
    if (!std::filesystem::exists(technical_config_g.tapo_pipe_name)) {
        my_logger_g.logger->error("The pipe {} does not exist", technical_config_g.tapo_pipe_name);
        return false;
    }
    // std::ofstream pipe;
    // pipe.open(technical_config_g.tapo_pipe_name);
    // if (pipe) {
    // pipe << cmd << "\n";
        // } else {
        // my_logger_g.logger->warn("Failed to open pipe: {}", technical_config_g.tapo_pipe_name);
        // return false;
        // }
    system(("sudo echo \"" + cmd + "\" > /tmp/tapo_pipe").c_str());
    return true;
}
