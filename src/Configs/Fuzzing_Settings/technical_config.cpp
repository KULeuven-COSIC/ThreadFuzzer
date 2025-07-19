#include "Configs/Fuzzing_Settings/technical_config.h"

#include "helpers.h"

Technical_Config technical_config_g;

std::ostream& Technical_Config::dump(std::ostream &os) const
{
    os << "socket_1: " << socket_1 << std::endl;
    os << "socket_2: " << socket_2 << std::endl;
    os << "interface: " << interface << std::endl;
    os << "network_dataset" << network_dataset << std::endl;
    return os;
}

std::ostream &operator<<(std::ostream &os, const Technical_Config &config)
{
    return config.dump(os);
}