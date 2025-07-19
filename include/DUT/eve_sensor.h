#pragma once

#include <string>

#include "DUT/DUT_base.h"

class Eve_Sensor final : public DUT_Base {
public:
  ~Eve_Sensor() = default;
  bool start() override;
  bool stop() override;
  bool restart() override;
  bool is_running() override;
  bool reset() override;
  bool factoryreset() override;
  void prime_reboot_nb(const int nb);

private:
  bool eve_to_pipe(const std::string cmd);
  int nb_of_reboots = 4;
  std::string ask_chip(const char *cmd);
};
