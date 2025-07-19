#include "Protocol_Stack/RCP/RCP_Sim.h"

#include "helpers.h"
#include "my_logger.h"
#include "Configs/Fuzzing_Settings/technical_config.h"

#include <iostream>

extern Technical_Config technical_config_g;
extern My_Logger my_logger_g;

bool RCP_Sim::start() {
    const std::string socket = technical_config_g.socket_1;

    /* Make sure no RCP session is running */
    this->stop();

    /* Start RCP */
    if (helpers::create_screen_session(name_)) {
        std::cout << "Failed to create a screen session for RCP" << std::endl;
        return false;
    }

    const std::string start_rcp_cmd = "./third-party/openthread/build/simulation/examples/apps/ncp/" + name_ + " 1 > " + socket + " < " + socket;
    if (helpers::stuff_cmd_to_screen(name_, start_rcp_cmd)) {
        std::cout << "Failed to start RCP" << std::endl;
        return false;
    }

    return true;
}

bool RCP_Sim::stop() {

    /* Kill the process */
    if (!helpers::kill_process(name_)) {
        my_logger_g.logger->warn("Failed to kill {}", name_);
    }

    /* Stop screen session */
    if (helpers::stop_screen_session(name_)) {
        std::cout << "Failed to stop RCP" << std::endl;
        my_logger_g.logger->warn("Failed to stop RCP");
        return false;
    }
    return true;
}

bool RCP_Sim::restart() {
    if (!this->stop()) return false;
    std::this_thread::sleep_for(std::chrono::seconds(1));
    if (!this->start()) return false;
    return true;
}

bool RCP_Sim::is_running() {
    return helpers::is_process_alive(name_);
}