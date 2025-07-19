#include "Communication/ot_pipe_helper.h"
#include "Protocol_Stack/OT_Instances/OT_types.h"
#include "helpers.h"
#include <cerrno>
#include <cstdint>
#include <ctime>
#include <fcntl.h>
#include <iostream>
#include <spdlog/logger.h>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <thread>
#include <time.h>

extern My_Logger my_logger_g;
// make sure that the
OT_Pipe_Helper ot_pipe_helper_g;

/**
   constructor to initialize sockets and store the ports
   - Also binds the "return" port

   TODO: remove constructor, init values in header!
 */
OT_Pipe_Helper::OT_Pipe_Helper() {

  std::cout << "created helper object" << std::endl;

  SocketHolder dut_holder = {-1, -1};
  SocketHolder pg_holder = {-1, -1};

  // create named pipes
  // std::vector<std::string> pipes = {std::to_string(base + dut_id) + ".pipe",
  //                                   std::to_string(base + pg_id) + ".pipe"};

  // create pipes, if possible
  // for (auto pipe : pipes) {
  //   if (mkfifo(pipe.c_str(), 0666) < 0) {
  //     std::cout << "COULD NOT CREATE " << pipe << std::endl;
  //     my_logger_g.logger->error("could not create pipe: {}", pipe);
  //     exit(0);
  //   }
  //   std::cout << "created " << pipe << std::endl;
  // }

  // std::cout << "created pipes" << std::endl;

  // std::cout << "configured pipes non-blockingly" << std::endl;

  // // open the pipes
  // dut_holder.fd = open(pipes[0].c_str(), O_WRONLY);
  // if (dut_holder.fd < 0) {
  //   std::cout << "COULD NOT OPEN " << pipes[0] << std::endl;
  //   my_logger_g.logger->error("could not create pipe: {}", pipes[0]);
  //   exit(0);
  // }

  // std::cout << "opened dut_holder.fd" << std::endl;

  // if ((pg_holder.fd = open(pipes[1].c_str(), O_WRONLY)) < 0) {
  //   std::cout << "COULD NOT OPEN " << pipes[1] << std::endl;
  //   my_logger_g.logger->error("could not create pipe: {}", pipes[0]);
  //   exit(0);
  // }
  // if ((dut_holder.ret_fd = open(pipes[2].c_str(), O_RDONLY)) < 0) {
  //   std::cout << "COULD NOT OPEN " << pipes[2] << std::endl;
  //   my_logger_g.logger->error("could not create pipe: {}", pipes[0]);
  //   exit(0);
  // }
  // // make readers non-blocking. TODO: check if this is even necessary...
  // fcntl(dut_holder.fd, F_SETFL, O_NONBLOCK);
  // if ((pg_holder.ret_fd = open(pipes[3].c_str(), O_RDONLY)) < 0) {
  //   std::cout << "COULD NOT OPEN " << pipes[3] << std::endl;
  //   my_logger_g.logger->error("could not create pipe: {}", pipes[0]);
  //   exit(0);
  // }
  // fcntl(pg_holder.fd, F_SETFL, O_NONBLOCK);

  // std::cout << "opened pipes" << std::endl;

  // fill in the records
  dut_holder_ = dut_holder;
  pg_holder_ = pg_holder;
}

/**
   destructor to break sockets down again at shutdown
*/
OT_Pipe_Helper::~OT_Pipe_Helper() {
  // std::cout << "closing the pipes!" << std::endl;
  close(dut_holder_.fd);
  close(pg_holder_.fd);
  close(dut_holder_.ret_fd);
  close(pg_holder_.ret_fd);
}

/**
   Sends a packet to socket/address in sock_holder
 */
bool OT_Pipe_Helper::node_send_simple(OT_TYPE ot_type, std::string payload) {
  SocketHolder sock_holder;
  if (ot_type == OT_TYPE::DUT)
    sock_holder = dut_holder_;
  else
    sock_holder = pg_holder_;

  int n = write(sock_holder.fd, payload.c_str(), payload.length());
  if (n < 0) {
    my_logger_g.logger->error("failed to send packet \"{}\" to {} (error code: {}, pipe: {})", payload,
                              helpers::get_name_prefix_by_ot_type(ot_type), errno, sock_holder.fd);
  }

  return true;
}

std::future<std::vector<std::string>>
OT_Pipe_Helper::node_async_reader(OT_TYPE ot_type, int timeout_micros,
                                  std::string tmplate) {
  SocketHolder sock_holder;

  if (ot_type == OT_TYPE::DUT)
    sock_holder = dut_holder_;
  else
    sock_holder = pg_holder_;

  std::future<std::vector<std::string>> future_lines =
      std::async(node_async_worker, sock_holder, timeout_micros, tmplate);

  return future_lines;
}

/**
   TODO: run this worker with a "marker" like "thread start\r\nDone", and if
   this is found, we break the loop
 */
std::vector<std::string>
OT_Pipe_Helper::node_async_worker(const SocketHolder sock_holder,
                                  int timeout_millis,
                                  const std::string tmplate) {
  // std::cout << "running worker" << std::endl;
  // my_logger_g.logger->info("starting worker");

  // lets receive
  char ret_buffer[512];
  std::string fullstring = "";

  // int t = 0;
  std::vector<std::string> lines = {};

  int ret = -1;

  std::clock_t t = std::clock();

  // int t = 0;


  while (true) {
    // std::cout << "diff = " << ((float)(std::clock() - t)) / CLOCKS_PER_SEC
    //           << "sec at " << CLOCKS_PER_SEC << " clk/s" << std::endl;
    if (std::clock() - t > (CLOCKS_PER_SEC * timeout_millis / 1000)) {
      std::cout << "passed timeout of " << timeout_millis << "ms"<< std::endl;
      break;
    }

    ret = read(sock_holder.ret_fd, ret_buffer, sizeof(ret_buffer) - 1);
    if (ret < 0 && errno != EAGAIN) {
      std::cout << "failed to read pipe " << sock_holder.ret_fd
                << ", ret: " << ret << " with "
                << " errno: " << errno << std::endl;
    }
    /* NOTE: this part is dangerous!! perform timeout check before this! */
    if (ret < 0 && errno == EAGAIN) {
      continue;
    }

    ret_buffer[ret] = '\0';
    fullstring = fullstring + std::string(ret_buffer);

    if (fullstring.find(tmplate) != std::string::npos) {
      // std::cout << "\"" << tmplate << "\"found at " << (std::clock() - t)
      //            << " at sec " << ((float)(std::clock() - t)) / CLOCKS_PER_SEC
      //            << std::endl;
      // my_logger_g.logger->info("found!!");
      break;
    }


    std::this_thread::sleep_for(std::chrono::microseconds(1));

    // std::cout << now_time - start_time << std::endl;
  }

  // auto end_time = std::chrono::steady_clock::now();
  // my_logger_g.logger->info("worker time spend is {}", (end_time - start_time)
  // / 1000000);

  std::stringstream ss(fullstring);
  std::string tmp;
  while (std::getline(ss, tmp, '\n')) {
    lines.push_back(tmp);
  }

  return lines;
}

/**
   Sends an package, and returns true if ANY datagram is received in
   return.


 */
bool OT_Pipe_Helper::node_heartbeat(OT_TYPE type, int timeout_micros) {
  // SocketHolder sock_holder;
  // if (type == OT_TYPE::DUT)
  //   sock_holder = dut_holder_;
  // else
  //   sock_holder = pg_holder_;

  // if (!udp_send_simple(type, "\n"))
  //   return false;

  //

  return true;
}

/**
   Helper function to open named pipes.
   NOTE: this function will block without a "reader" (a DUT) present.
   Reason: if you open pipe-end with writer only, then it will wait
   for a reader on the other end.
 */
bool OT_Pipe_Helper::open_pipes(OT_TYPE ot_type, int node_id) {

  // open writer port
  int fd = open(("in_" + std::to_string(node_id) + ".pipe").c_str(), O_WRONLY);
  // fcntl(fd, F_SETFL, O_NONBLOCK);
  if (fd < 0) {
    std::cout << "COULD NOT OPEN "
              << "in_" << node_id << ".pipe" << std::endl;
    my_logger_g.logger->error("could not open in pipe for {}", node_id);
    return false;
  }
  std::cout << "opened in pipe to " << node_id << std::endl;

  // open reader port
  int ret_fd =
      open(("out_" + std::to_string(node_id) + ".pipe").c_str(), O_RDONLY);
  fcntl(ret_fd, F_SETFL, O_NONBLOCK);
  if (ret_fd < 0) {
    std::cout << "COULD NOT OPEN "
              << "out_" << node_id << ".pipe" << std::endl;
    my_logger_g.logger->error("could not open out pipe for {}", node_id);
    return false;
  }
  std::cout << "opened out pipe to " << node_id << std::endl;

  // assign to structure for safekeeping
  if (ot_type == OT_TYPE::DUT) {
    dut_holder_.fd = fd;
    dut_holder_.ret_fd = ret_fd;
  } else {
    pg_holder_.fd = fd;
    pg_holder_.ret_fd = ret_fd;
  }
  return true;
}

void OT_Pipe_Helper::close_pipes(OT_TYPE ot_type) {
  if (ot_type == OT_TYPE::DUT) {
    if (close(dut_holder_.fd) < 0) {
      my_logger_g.logger->debug("Failed to close dut fd: {} (errno={})", dut_holder_.fd, errno);
    }
    if (close(dut_holder_.ret_fd) < 0) {
      my_logger_g.logger->debug("Failed to close dut ret_fd: {} (errno={})", dut_holder_.ret_fd, errno);
    }
  } else {
    if (close(pg_holder_.fd) < 0) {
      my_logger_g.logger->debug("Failed to close pg fd: {} (errno={})", pg_holder_.fd, errno);
    }
    if (close(pg_holder_.ret_fd) < 0) {
      my_logger_g.logger->debug("Failed to close pg ret_fd: {} (errno={})", pg_holder_.ret_fd, errno);
    }
  }
}
