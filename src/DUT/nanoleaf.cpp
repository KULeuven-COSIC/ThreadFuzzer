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
    bool r = echo_to_pipe("on");
    if (r) {
      std::cout << "[DUT]: NANOLEAF: started succesfully..." << std::endl;
      std::cout << "[DUT]: waiting 10 seconds for stability" << std::endl;
      my_logger_g.logger->debug("[DUT]: NANOLEAF: waiting for 10 seconds for stability");
      std::this_thread::sleep_for(std::chrono::seconds(10));
      std::cout << "[DUT]: DonE" << std::endl;
    }
    return r;
}

bool Nanoleaf::stop() {
   my_logger_g.logger->debug("[DUT]: NANOLEAF: STOPPING"); 
   return echo_to_pipe("off");
}

bool Nanoleaf::restart() {
    my_logger_g.logger->debug("[DUT]: NANOLEAF: RESTARTING");
    std::cerr << "[DUT]: NANOLEAF: RESTARTING" << std::endl;
    if (!stop())
	    return false;
    for (int i = 0; i < 3; i++) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
      std::cout << "[DUT]: NANOLEAF powered off for: " << i + 1 << std::endl;
    }
    if (!start())
      return false;
    
    factoryreset();
    std::cout << "[DUT]: NANOLEAF RESTART SUCCES" << std::endl;
    
    return true;
}

bool Nanoleaf::is_running() {
    return true;
}

// called after each iteration... (no touch-e!!)
bool Nanoleaf::reset()
{
    my_logger_g.logger->debug("[DUT]: NANOLEAF: RESETTING");
    std::cerr << "[DUT]: NANOLEAF RESETTING " << std::endl;
    if (!stop())
      return false;
    // std::this_thread::sleep_for(std::chrono::seconds(3));
    for (int i = 0; i < timers_config_g.tapo_restart_wait_time_s; i++) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
      std::cout << "[DUT]: NANOLEAF powered off for: " << i + 1 << std::endl;
    }
    if (!start())
      return false;
    std::cout << "[DUT]: NANOLEAF RESET SUCCESS" << std::endl;
    return true;
  
}

// only called at the start (normally)
bool Nanoleaf::factoryreset()
{
    std::cout << "[NANOLEAF]: FACTORYRESET" << std::endl;
    std::cout << "going: ";
    bool tr = true;
    for (int i = 0; i < 5; i++) {
      bool pof = echo_to_pipe("off");
      std::this_thread::sleep_for(std::chrono::seconds(1));
      std::cout << "WAITED fOR " << timers_config_g.tapo_restart_wait_time_s << std::endl;
      bool pon = echo_to_pipe("on");
      tr = tr && pof && pon;
      std::cout << i << " ";
    }
    std::cout << std::endl;
    std::cout << "[NANOLEAF]: FR COMPLETE" << std::endl;
    if (!tr)
      std::cout << "[DUT]: NANOLEAF: failure detected during restarting!" << std::endl;
    std::cout << "[DUT]: [NANOLEAF]: give NANOLEAF 20 seconds to settle" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(20));
    return tr;
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
