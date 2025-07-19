#pragma once

#include "Protocol_Stack/OT_Instances/OT_types.h"
#include "my_logger.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <future>
#include <vector>

struct SocketHolder {
  int fd;           // socket
  int ret_fd;
};

/**
   Helper class to perform the UDP request/responses on simulated OpenThread
   nodes, inspired by OTNS simulator.
 */
class OT_Pipe_Helper {
public:
  OT_Pipe_Helper();
  ~OT_Pipe_Helper();
  bool node_send_simple(OT_TYPE ot_type, std::string payload);
  // bool udp_send_and_expect(OT_TYPE type, uint64_t delay, uint8_t cmd,
  //                          std::string payload, std::string substring_to_match,
  //                          int timeout_micros, int max_lines);
  // std::string udp_getline_till_prompt(SocketHolder sock_holder,
  //                                    int timeout_micros);
  bool node_heartbeat(OT_TYPE type, int timeout_micros);
  // bool run_commands_expect_done(std::vector<std::string> cli_commands,
  //                               OT_TYPE type);
  // bool run_commands_simple(std::vector<std::string> cli_commands,
  //                               OT_TYPE type);
  bool open_pipes(OT_TYPE type, int node_id);
  void close_pipes(OT_TYPE type);

  std::string extracted(std::thread &node_thread);
  std::future<std::vector<std::string>>
  node_async_reader(OT_TYPE ot_type, int timeout_micros,
                    std::string tmplate);
  static std::vector<std::string>
  node_async_worker(const SocketHolder sock_holder, int timeout_millis,
                    const std::string tmplate);

  // addresses
  SocketHolder dut_holder_;
  SocketHolder pg_holder_;

private:
};
