#include "Protocol_Stack/OT_Instances/OT_dummy.h"

bool OT_Dummy::start() {
    return true;
}

bool OT_Dummy::stop() {
    return true;
}

bool OT_Dummy::restart() {
    return true;
}

bool OT_Dummy::is_running() {
    return true;
}

bool OT_Dummy::reset()
{
    return true;
}

bool OT_Dummy::activate_thread()
{
    return true;
}

bool OT_Dummy::deactivate_thread() { return true; }

bool OT_Dummy::factoryreset() { return true; }
