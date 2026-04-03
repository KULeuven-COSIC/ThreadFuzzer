#pragma once

#include <string>

#include "DUT/DUT_base.h"

class Generic_Sensor : public DUT_Base {
public:
  explicit Generic_Sensor(const std::string& name, int fr_duration_s);
  virtual ~Generic_Sensor() = default;
  virtual bool start();
  virtual bool stop();
  virtual bool restart();
  virtual bool is_running();
  virtual bool reset();
  virtual bool factoryreset();

protected:
  std::string name_;
  int fr_duration_s_;
};
