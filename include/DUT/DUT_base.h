#pragma once

#include <string>

class DUT_Base {
public:
    virtual ~DUT_Base() = default;
    virtual bool start() = 0;
    virtual bool stop() = 0;
    virtual bool restart();
    virtual bool is_running() = 0;
    virtual bool reset() = 0;
  virtual bool factoryreset() = 0;
};
