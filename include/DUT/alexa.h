#pragma once

#include <string>

#include "DUT/DUT_base.h"

class Alexa : public DUT_Base {
public:
    Alexa();
    ~Alexa();
    bool start() override;
    bool stop() override;
    bool restart() override;
    bool is_running() override;
  bool reset() override;
  bool factoryreset() override;

private:
    void power_on();
    void power_off();
    void restart(int wait_time_s);
    bool echo_to_pipe(const std::string cmd);
};
