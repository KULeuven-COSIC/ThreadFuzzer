#pragma once

#include <string>
#include <vector>

#include <nlohmann/json.hpp>

class Technical_Config {
public:
  std::string socket_1;
  std::string socket_2;
  std::string interface;

  std::string network_dataset =
      "0e08000000000001000000030000174a0300001035060004001fffe00708fd1e234fcc"
      "a6183b0c0402a0f7f80102dead0208dead1111dead2222030d4a616b6f6273506c6179"
      "50656e051011112233445566778899dead1111dead0410209f8ccb50f556da46166ef4"
      "fdcbed4a";

  std::string tapo_pipe_name = "/tmp/tapo_pipe";
  std::string eve_pipe_name = "/dev/ttyUSB0";

  std::string ot_path_for_dut;
  std::string ot_path_for_packet_generator = "";

  std::ostream &dump(std::ostream &os) const;
};

std::ostream &operator<<(std::ostream &os, const Technical_Config &config);

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Technical_Config, socket_1,
                                                socket_2, interface,
                                                network_dataset, tapo_pipe_name,
                                                ot_path_for_dut,
                                                ot_path_for_packet_generator)
