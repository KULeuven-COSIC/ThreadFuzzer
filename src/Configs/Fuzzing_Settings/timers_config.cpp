#include "Configs/Fuzzing_Settings/timers_config.h"

#include "helpers.h"

Timers_Config timers_config_g;

std::ostream& Timers_Config::dump(std::ostream &os) const
{
    os << "iteration_length_s: " << iteration_length_ms << std::endl;
    os << "router_selection_jitter" << router_selection_jitter_s << std::endl;
    os << "speed" << speed << std::endl;
    return os;
}

std::ostream &operator<<(std::ostream &os, const Timers_Config &config)
{
    return config.dump(os);
}
