#include "DUT/DUT_base.h"

#include "Configs/Fuzzing_Settings/timers_config.h"

#include <chrono>
#include <thread>

extern Timers_Config timers_config_g;

bool DUT_Base::restart() {
    stop();
    std::this_thread::sleep_for(std::chrono::seconds(timers_config_g.protocol_stack_restart_timer_s));
    if (!start()) return false;
    return true;
}