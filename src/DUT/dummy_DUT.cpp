#include "DUT/dummy_DUT.h"

#include <iostream>

bool Dummy_DUT::start() {
    return true;
}

bool Dummy_DUT::stop() {
    return true;
}

bool Dummy_DUT::restart() {
    return true;
}

bool Dummy_DUT::is_running() {
    return true;
}

bool Dummy_DUT::reset()
{
    return true;
}

bool Dummy_DUT::factoryreset() { return true; }
