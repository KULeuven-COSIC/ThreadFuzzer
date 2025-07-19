#include "helpers.h"

#include "shm/shared_memory.h"

#include "Configs/Fuzzing_Settings/technical_config.h"
#include "Configs/Fuzzing_Settings/timers_config.h"
#include "my_logger.h"

#include <algorithm>
#include <asm-generic/errno-base.h>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <iostream>
#include <random>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>

// #include <sys/wait.h>

extern Technical_Config technical_config_g;
extern Timers_Config timers_config_g;
extern My_Logger my_logger_g;

static constexpr int TIMEOUT_CODE = 124;

namespace helpers {

std::string get_openthread_path_by_ot_type(OT_TYPE ot_type) {
  switch (ot_type) {
  case OT_TYPE::PACKET_GENERATOR:
    return technical_config_g.ot_path_for_packet_generator;
  case OT_TYPE::DUT:
    return technical_config_g.ot_path_for_dut;
  }
  throw std::runtime_error("Unknown OT_TYPE");
}

std::string get_name_prefix_by_ot_type(OT_TYPE ot_type) {
  switch (ot_type) {
  case OT_TYPE::PACKET_GENERATOR:
    return "PG-";
  case OT_TYPE::DUT:
    return "DUT-";
  }
  throw std::runtime_error("Unknown OT_TYPE");
}

std::string read_file_last_chars(const std::string &filename, int char_count) {
  std::cout << "char count " << char_count << std::endl;
  std::ifstream file(filename,
                     std::ios::in | std::ios::ate); // Open file at the end
  if (!file.is_open()) {
    my_logger_g.logger->warn("Could not open file: {}", filename);
    return {};
  }

  std::streampos file_size = file.tellg(); // Get the file size
  if (file_size == 0) {
    return ""; // Empty file
  }

  // Calculate the position to start reading
  std::streampos start_pos =
      (file_size > static_cast<std::streampos>(char_count))
          ? file_size - static_cast<std::streampos>(char_count)
          : 0;

  // Seek to the start position
  file.seekg(start_pos);

  // Read the content from the start position to the end
  std::string result;
  result.reserve(char_count);
  result.assign(std::istreambuf_iterator<char>(file),
                std::istreambuf_iterator<char>());

  file.close();
  return result;
}

int exec_system_cmd_with_timeout(const std::string &cmd, int timeout_s) {
  std::string timeout = "timeout " + std::to_string(timeout_s) + "s ";
  std::string cmd_with_timeout;

  static const std::string sudo_keyword = "sudo";
  if (cmd.find(sudo_keyword) == 0) {
    /* if the command starts with sudo, put the timeout in between sudo and the
     * rest of the command */
    cmd_with_timeout =
        sudo_keyword + " " + timeout + cmd.substr(sudo_keyword.length() + 1);
  } else {
    cmd_with_timeout = timeout + cmd;
  }
  cmd_with_timeout += " &> /dev/null";
  int ret = std::system(cmd_with_timeout.c_str());
  if (ret == TIMEOUT_CODE)
    my_logger_g.logger->warn("Command \"{}\" timed out", cmd);
  return ret;
}

int exec_system_cmd_with_default_timeout(const std::string &cmd) {
  return exec_system_cmd_with_timeout(
      cmd, timers_config_g.system_cmd_max_timeout_default_s);
}

int create_screen_session(const std::string &screen_name,
                          const std::string &log_file_path) {
  std::string cmd;
  if (log_file_path.length() == 0) {
    cmd = "screen -dmS \"" + screen_name + "\" -t \"" + screen_name +
          "\" > /dev/null 2> create_screen_errors.txt";
  } else {
    cmd = "screen -L -Logfile \"" + log_file_path + "\" -dmS \"" + screen_name +
          "\" -t \"" + screen_name +
          "\" > /dev/null 2> create_screen_errors.txt";
  }
  return exec_system_cmd_with_default_timeout(cmd);
}

int screen_session_exists(const std::string &screen_name) {
  const std::string cmd =
      "screen -S \"" + screen_name + "\" -X select . ; echo $?";
  return exec_system_cmd_with_default_timeout(cmd);
}

int stop_screen_session(const std::string &screen_name) {
  const std::string cmd = "screen -S \"" + screen_name + "\" -X quit" +
                          " > /dev/null 2> stop_screen_errors.txt";
  return exec_system_cmd_with_default_timeout(cmd);
}

int stuff_cmd_to_screen(const std::string &screen_name,
                        const std::string &cmd) {
  const std::string full_cmd =
      "screen -S \"" + screen_name + "\" -X stuff \"" + cmd + "\r\n\"";
  return exec_system_cmd_with_default_timeout(full_cmd);
}

int execute_screen_cmd(const std::string &cmd) {
  return exec_system_cmd_with_default_timeout(cmd);
}

// NOTE: no longer used, in favor of kill_pid
bool kill_process(const std::string &process_name) {
  std::cout << "KILLING " << process_name << std::endl;
  const std::string cmd =
      "sudo killall -9 \"" + process_name + "\" > /dev/null 2> /dev/null";
  if (exec_system_cmd_with_default_timeout(cmd) != 0) {
    return false;
  }
  return true;
}

bool kill_pid(const std::string &pid_name) {
  const std::string cmd =
      "sudo kill -9 \"" + pid_name + "\" > /dev/null 2> /dev/null";
  if (exec_system_cmd_with_default_timeout(cmd) != 0) {
    // std::cout << "KILLING " << pid_name  << " FAILED " << std::endl;
    return false;
  }
  return true;
}

bool is_pid_alive(const std::string &pid_name) {
  const std::string cmd =
      "sudo kill -0 \"" + pid_name + "\" > /dev/null 2> /dev/null";
  if (exec_system_cmd_with_default_timeout(cmd) != 0) {
    return false;
  }
  return true;
}

/* TODO: dont hardcode ports! */
bool send_udp_heartbeat(int port) {
  int fd;
  if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    std::cout << "COULD NOT CREATE A SOCKET!!" << std::endl;
    my_logger_g.logger->warn("could not create socket");
    return false;
  }
  fcntl(fd, F_SETFL, O_NONBLOCK);

  int fdret;
  if ((fdret = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    std::cout << "COULD NOT CREATE A SOCKET!!" << std::endl;
    my_logger_g.logger->warn("could not create socket");
    close(fd);
    return false;
  }
  fcntl(fdret, F_SETFL, O_NONBLOCK);

  sockaddr_in retAddr;
  memset((char *)&retAddr, 0, sizeof(retAddr));
  retAddr.sin_family = AF_INET;
  retAddr.sin_port = htons(9000);
  inet_pton(AF_INET, "127.0.0.1", &retAddr.sin_addr);

  /* make sure the return address is reserved for us */
  if (bind(fdret, (const struct sockaddr *)&retAddr, sizeof(retAddr)) < 0) {
    std::cout << "COULD NOT BIND RETURN ADDRESS" << std::endl;
    my_logger_g.logger->warn("could not bind socket to address!");
    close(fd);
    close(fdret);
    return false;
  }

  uint8_t buf[11] = {
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, // time delay
      0x00, // UART write
      0x00, // 2B size in little endian!!
      0x00,
  };

  sockaddr_in destAddr;
  memset((char *)&destAddr, 0, sizeof(destAddr));
  destAddr.sin_family = AF_INET;
  destAddr.sin_port = htons(9000 + port);
  inet_pton(AF_INET, "127.0.0.1", &destAddr.sin_addr);

  for (auto c : buf) {
    std::cout << c << " ";
    if (c == '\0')
      break;
  }
  std::cout << std::endl;

  /* "<<<Azrael, are you dead?>>>" */
  ssize_t sentBytes =
      sendto(fd, buf, 11, 0, reinterpret_cast<sockaddr *>(&destAddr),
             sizeof(destAddr));
  if (sentBytes < 0) {
    std::cout << "FAILED TO SEND PACKET" << std::endl;
    my_logger_g.logger->warn("failed to send packet");
    return false;
  }

  /* "<<<Miauw>>>" */
  socklen_t len = sizeof(retAddr);
  char buffer[1024];

  int tries = 0;

  int n = -1;
  while (tries < 50) {
    n = recvfrom(fdret, buffer, 1024, 0, reinterpret_cast<sockaddr *>(&retAddr),
                 &len);
    tries++;
    std::this_thread::sleep_for(std::chrono::microseconds(1));
    if (errno != EAGAIN || n > -1)
      break;
  }

  close(fd);
  close(fdret);

  if (n < 0) {
    std::cout << "no response from " << port + 9000 << std::endl;
    std::cout << "error is " << n << std::endl;
    my_logger_g.logger->warn("response code was: {}", errno);
    return false;
  }

  buffer[n] = '\0';
  std::cout << "Server :" << buffer << ": End Server" << std::endl;

  return true;
}

/* TODO avoid creating a socket over and over again!! */
bool send_udp_request(const std::string msg, int port) {
  int fd;

  if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    std::cout << "COULD NOT CREATE A SOCKET!!" << std::endl;
    return false;
  }

  uint8_t buf[1024] = {
      0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00,
      0x00,                         // time delay
      0x02,                         // UART write
      0x06,                         // 2B size in little endian!!
      0x00, 0x73, 0x74, 0x61, 0x65, // message (just state here)
      0x0D,                         // carriage return
      0x0A,                         // newline
  };

  // put in the message size
  buf[9] = msg.length();

  // then the msg itself
  for (int i = 0; i < msg.length(); i++) {
    buf[9 + i] = msg.c_str()[i];
  }

  // followed by a footer
  buf[9 + msg.length()] = '\r';
  buf[9 + msg.length() + 1] = '\n';
  buf[9 + msg.length() + 2] = '\0';

  sockaddr_in destAddr;

  memset((char *)&destAddr, 0, sizeof(destAddr));
  destAddr.sin_family = AF_INET;
  destAddr.sin_port = htons(port + 9000);

  inet_pton(AF_INET, "127.0.0.1", &destAddr.sin_addr);

  for (auto c : buf) {
    std::cout << c << " ";
    if (c == '\0')
      break;
  }
  std::cout << std::endl;

  ssize_t sentBytes =
      sendto(fd, buf, 9 + msg.length() + 3, 0,
             reinterpret_cast<sockaddr *>(&destAddr), sizeof(destAddr));
  close(fd);

  if (sentBytes < 0) {
    std::cout << "FAILED TO SEND PACKET" << std::endl;
    return false;
  } else {
    std::cout << "Sent " << sentBytes << "B" << std::endl;
  }

  return true;
}

// NOTE: no longer used, in favor of is_pid_alive
bool is_process_alive(const std::string &process_name) {

  const std::string cmd =
      "sudo killall -s 0 \"" + process_name + "\" > /dev/null 2> /dev/null";
  if (exec_system_cmd_with_default_timeout(cmd) != 0) {
    std::cout << "PROCESS " << process_name << " IS DEAD!!" << std::endl;
    my_logger_g.logger->error("PROCESS IS DEAD!!");
    return false;
  }
  // std::cout << "PROCESS " << process_name << " IS ALIVE!!!" << std::endl;
  return true;
}

bool signal_service(const std::string &service_name,
                    const std::string &signal) {
  const std::string cmd =
      "sudo systemctl kill --signal=" + signal + " " + service_name;
  if (exec_system_cmd_with_default_timeout(cmd) != 0) {
    return false;
  }
  return true;
}

bool check_if_path_exists(const std::filesystem::path &path) {
  try {
    if (!std::filesystem::exists(path))
      return false;
  } catch (std::exception &ex) {
    std::cerr << "Caught exception during check_if_path_exists(): " << ex.what()
              << std::endl;
    return false;
  }
  return true;
}

bool delete_if_file_is_empty(const std::filesystem::path &path) {
  if (!std::filesystem::exists(path) || !std::filesystem::is_regular_file(path)) {
    return false;
  }

  if (std::filesystem::file_size(path) != 0) {
    return false;
  }

  if (!std::filesystem::remove(path)) {
    my_logger_g.logger->warn("Failed to remove file {}", path);
    return false;
  }

  return true;
}

bool set_permissions_if_path_exists(const std::filesystem::path &path, std::filesystem::perms perms) {
  try {
    if (std::filesystem::exists(path)) {
      std::filesystem::permissions(path, perms);
    } else {
      my_logger_g.logger->debug("Path does not exist: {}", path);
      return false;
    }
  } catch (std::exception &ex) {
    my_logger_g.logger->warn("Caught exception during set_permissions_if_path_exists() {}", ex.what());
    std::cerr << "Caught exception during set_permissions_if_path_exists(): " << ex.what()
              << std::endl;
    return false;
  }
  return true;
}

bool create_directories_if_not_exist(const std::filesystem::path &path) {
  try {
    /* First check if the directories exist */
    if (check_if_path_exists(path))
      return true;
    /* If doesn't exist, try to create the directory */
    if (!std::filesystem::create_directories(path))
      return false;
    /* Set the permissions */
    std::filesystem::permissions(path, std::filesystem::perms::all);
  } catch (std::exception &ex) {
    std::cerr << "Caught exception during create_directories_if_not_exist: "
              << ex.what() << std::endl;
    return false;
  }
  return true;
}

bool clear_instrumentation_files(const std::filesystem::path &path) {
  // check if path exists, just return if not
  if (check_if_path_exists(path))
    return true;
  // clear gcda files recursively, starting from the path
  const std::string cmd =
      "find " + path.string() + "-type f -name '*.gcda' -delete";
  if (exec_system_cmd_with_default_timeout(cmd) != 0) {
    return false;
  }
  std::cerr << "cleared instr files for " << path << std::endl;
  return true;
}

bool run_screen_cli_commands(const std::string session_name,
                             const std::vector<std::string> cli_commands) {
  for (auto cmd : cli_commands) {
    if (helpers::stuff_cmd_to_screen(session_name, cmd)) {
      my_logger_g.logger->error("failed to run command " + cmd + " on " +
                                session_name);
      return false;
    } else {
      my_logger_g.logger->debug("ran " + cmd + " on " + session_name);
    }
  }

  return true;
}

bool run_udp_cli_commands(const std::vector<std::string> cli_commands,
                          int port) {
  for (auto cmd : cli_commands) {
    if (!helpers::send_udp_request(cmd, port)) {
      my_logger_g.logger->error("failed to run command " + cmd + " on " +
                                std::to_string(port));
      return false;
    } else {
      my_logger_g.logger->debug("ran " + cmd + " on " + std::to_string(port));
    }
  }

  return true;
}

// NOTE: returns the final line returned by a execution of a shell command.
// std::string shell_command(const std::string cmd) {
//   char psBuffer[128];
//   FILE *iopipe;
//
//   if ((iopipe = popen(cmd.c_str(), "r")) == NULL)
//     exit(1);
//   while (!feof(iopipe)) {
//     // NOTE: this will fetch output line per line (\n ending!)
//     // -> keeping the LAST line!!
//     if (fgets(psBuffer, 128, iopipe) != NULL) {
//       // puts(psBuffer);
//     }
//   }
//   auto flg = pclose(iopipe);
//
//   std::cout << cmd << " returned " << flg << std::endl;
//   // NOTE: string fetched has already a newline
//   // std::cout << "and this string: " << std::string(psBuffer);
//
//   if (!flg)
//     return std::string(psBuffer);
//   else
//     return "";
// }

uint64_t EXP0(uint64_t limit, double lambda) {
  static std::random_device rd;
  std::exponential_distribution<double> distribution(lambda);
  uint64_t rn;
  do {
    rn = (uint64_t)distribution(rd);
  } while (rn >= limit);
  return rn;
}

double URD(double start, double end) {
  static std::random_device rd;
  std::uniform_real_distribution<> distribution(start, end);
  return distribution(rd);
}

uint64_t UR(uint64_t start, uint64_t end) {
  static std::random_device rd;
  std::uniform_int_distribution<long long unsigned> distribution(start, end - 1);
  return (uint64_t)distribution(rd);
}

uint64_t UR0(uint64_t limit) {
  // if (limit == 0) return 0;
  if (limit == 0)
    throw std::runtime_error("UR0(0)");
  if (limit == 1)
    return 0;
  return UR(0, limit);
}

uint64_t UR1(uint64_t limit) {
  if (limit == 1)
    return 0;
  return UR(1, limit + 1);
}

std::string shorten_dissector_summary(const std::string &dissector_summary) {
  std::string result = "";
  bool in_brackets = false;
  for (char c : dissector_summary) {
    if (in_brackets) {
      if (c == ')') {
        in_brackets = false;
        continue;
      }
    } else {
      if (c == '|')
        break;
      if (c == '(') {
        in_brackets = true;
      } else if (c == ' ') {
        continue;
      } else {
        result += c;
      }
    }
  }
  return result;
}

bool is_state_being_fuzzed(const std::string &state_name) {
  if (!fuzz_strategy_config_g.states_to_fuzz.empty()) {
    return std::find(fuzz_strategy_config_g.states_to_fuzz.begin(),
                     fuzz_strategy_config_g.states_to_fuzz.end(),
                     state_name) != fuzz_strategy_config_g.states_to_fuzz.end();
  }
  if (!fuzz_strategy_config_g.states_not_to_fuzz.empty()) {
    return std::find(fuzz_strategy_config_g.states_not_to_fuzz.begin(),
                     fuzz_strategy_config_g.states_not_to_fuzz.end(),
                     state_name) ==
           fuzz_strategy_config_g.states_not_to_fuzz.end();
  }
  return true;
}

const std::string get_layer_name_by_idx(uint8_t idx) {
  if (idx == SHM_MUTEX_MLE)
    return "MLE";
  else if (idx == SHM_MUTEX_COAP)
    return "COAP";
  return "UNKNOWN";
}

std::vector<std::string> get_field_prefixes_by_layer_idx(uint8_t mutex_name) {
  if (mutex_name == SHM_MUTEX_MLE)
    return {""};
  else if (mutex_name == SHM_MUTEX_COAP)
    return {""};
  throw std::runtime_error(
      "(Get field prefix by layer idx) Invalid mutex value " +
      std::to_string(mutex_name));
}

std::string get_dissector_by_layer_idx(uint8_t mutex_name) {
  if (mutex_name == SHM_MUTEX_MLE)
    return "wpan"; // "mle"
  else if (mutex_name == SHM_MUTEX_COAP)
    return "wpan";
  throw std::runtime_error("Cannot get dissector: Unsupported protocol");
}

int chip_pair(const std::string &device) {
  std::string cmd = "sudo ../connectedhomeip/chip_pair.sh " + device;
  int ret = std::system(cmd.c_str());
  if (ret == TIMEOUT_CODE)
    my_logger_g.logger->warn("Command \"{}\" timed out", cmd);
  else if (ret)
    my_logger_g.logger->warn("Command \"{}\" failed with exit code: {}", cmd,
                             ret);
  return ret;
}

bool chip_unpair(const std::string &device) {
  std::string cmd = "sudo ../connectedhomeip/chip_unpair.sh " + device;
  int ret = std::system(cmd.c_str());
  if (ret == TIMEOUT_CODE)
    my_logger_g.logger->warn("Command \"{}\" timed out", cmd);
  else if (ret)
    my_logger_g.logger->warn("Command \"{}\" failed with exit code: {}", cmd,
                             ret);
  return ret;
}

} // namespace helpers
