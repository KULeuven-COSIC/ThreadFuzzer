#pragma once

#include <string>

#include "DUT/DUT_base.h"

class Dummy_DUT : public DUT_Base {
public:
    ~Dummy_DUT() = default;
    bool start() override;
    bool stop() override;
    bool restart() override;
    bool is_running() override;
    bool reset() override;
  bool factoryreset() override;
};
