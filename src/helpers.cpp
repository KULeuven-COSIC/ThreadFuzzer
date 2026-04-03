#include "helpers.h"

#include "DUT/DUT_base.h"
#include "DUT/DUT_factory.h"
#include "DUT/DUT_names.h"
#include "shm/shared_memory.h"

#include "Configs/Fuzzing_Settings/technical_config.h"
#include "Configs/Fuzzing_Settings/timers_config.h"
#include "my_logger.h"

#include <algorithm>
#include <asm-generic/errno-base.h>
#include <boost/process.hpp>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <functional>
#include <ios>
#include <iostream>
#include <random>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <termios.h>
#include <unistd.h>

#include <fstream>
#include <fmt/core.h>

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
      "sudo pkill -" + signal + " " + service_name;
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
  if (!std::filesystem::exists(path) ||
      !std::filesystem::is_regular_file(path)) {
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

bool set_permissions_if_path_exists(const std::filesystem::path &path,
                                    std::filesystem::perms perms) {
  try {
    if (std::filesystem::exists(path)) {
      std::filesystem::permissions(path, perms);
    } else {
      my_logger_g.logger->debug("Path does not exist: {}", path);
      return false;
    }
  } catch (std::exception &ex) {
    my_logger_g.logger->warn(
        "Caught exception during set_permissions_if_path_exists() {}",
        ex.what());
    std::cerr << "Caught exception during set_permissions_if_path_exists(): "
              << ex.what() << std::endl;
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
  std::uniform_int_distribution<long long unsigned> distribution(start,
                                                                 end - 1);
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

/* TODO: Change it to use the return value. */
bool chip_pair(int node_id, const std::string& passcode, const std::string& discriminator) {
    static const std::string thread_dataset_hex = 
        "0e08000000000001000000030000174a0300001035060004001fffe00708fd1e234fcca6"
        "183b0c0402a0f7f80102dead0208dead1111dead2222030d4a616b6f6273506c61795065"
        "6e051011112233445566778899dead1111dead0410209f8ccb50f556da46166ef4fdcbed"
        "4a";

    my_logger_g.logger->info("Initiating CHIP pairing sequence for Node ID: {}", node_id);

    const std::string program = "./connectedhomeip/out/chip-tool";
    const std::vector<std::string> args = {
        "pairing",
        "ble-thread",
        std::to_string(node_id),
        "hex:" + thread_dataset_hex,
        passcode,
        discriminator,
        "--bypass-attestation-verifier",
        "true",
        "--timeout",
        "500"
    };

    try {
        std::string output = execute_command_and_get_output(program, args, true);

        // Safely get the last 1000 characters (or less, if the output is unusually short)
        size_t tail_length = std::min<size_t>(output.length(), 1000);
        std::string_view tail(output.data() + output.length() - tail_length, tail_length);

        // Only search the tail end for fatal errors
        if (tail.find("CHIP Error") != std::string_view::npos || tail.find("Timeout") != std::string_view::npos) {
            my_logger_g.logger->warn("CHIP pairing failed or timed out for Node ID: {}", node_id);
            return false;
        }

        my_logger_g.logger->info("CHIP pairing successful for Node ID: {}", node_id);
    } catch (const std::exception& ex) {
        my_logger_g.logger->error("Failed to launch chip-tool process for Node ID {}: {}", node_id, ex.what());
        return false;
    }

    return true;
}

bool chip_unpair(int node_id) {
    my_logger_g.logger->info("Initiating CHIP unpair sequence for Node ID: {}", node_id);

    const std::string program = "./connectedhomeip/out/chip-tool";
    const std::vector<std::string> args = {
        "pairing",
        "unpair",
        std::to_string(node_id)
    };

    try {
        std::string output = execute_command_and_get_output(program, args, true);

        // Safely get the last 1000 characters (or less, if the output is unusually short)
        size_t tail_length = std::min<size_t>(output.length(), 1000);
        std::string_view tail(output.data() + output.length() - tail_length, tail_length);

        // Only search the tail end for fatal errors
        if (tail.find("CHIP Error") != std::string_view::npos || tail.find("Timeout") != std::string_view::npos) {
            my_logger_g.logger->warn("CHIP unpair failed or timed out for Node ID: {}", node_id);
            return false;
        }

        my_logger_g.logger->info("CHIP unpair successful for Node ID: {}", node_id);
    } catch (const std::exception& ex) {
        my_logger_g.logger->error("Failed to launch chip-tool process for Node ID {}: {}", node_id, ex.what());
        return false;
    }

    return true;
}

int chip_fetch_reboot_count(int node_id, int endpoint_id) {
    my_logger_g.logger->debug("Fetching reboot count for Node {}...", node_id);

    const std::string program = "./connectedhomeip/out/chip-tool";
    const std::vector<std::string> args = {
        "generaldiagnostics",
        "read",
        "reboot-count",
        std::to_string(node_id),
        std::to_string(endpoint_id)
    };

    int current_reboot_count = -1;

    try {
        std::string response = execute_command_and_get_output(program, args, true);
        
        const std::string prefix = "RebootCount: ";
        size_t pos = response.find(prefix);
        
        if (pos == std::string::npos) {
            throw std::runtime_error("Unexpected response format: 'RebootCount:' not found in output.");
        }
        
        std::string number_str = response.substr(pos + prefix.length());
        current_reboot_count = std::stoi(number_str);
        
    } catch (const std::exception& ex) {
        my_logger_g.logger->error("Failed to parse reboot count for Node {}: {}", node_id, ex.what());
        throw std::runtime_error(std::string("chip_fetch_reboot_count failed: ") + ex.what());
    }

    my_logger_g.logger->info("COLLECTED RBT CNT FOR NODE {}: {}", node_id, current_reboot_count);
    return current_reboot_count;
}

bool send_command_to_device(const std::string& device_path, const std::string& cmd) {
    my_logger_g.logger->debug("Preparing to send command to device: {}", device_path);

    if (!std::filesystem::exists(device_path)) {
        my_logger_g.logger->error("The device at {} does not exist.", device_path);
        return false;
    }

    // Open the device natively.
    // O_RDWR = Read/Write, O_NOCTTY = No controlling terminal, O_NDELAY = Non-blocking
    int fd = open(device_path.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd == -1) {
        my_logger_g.logger->error("Failed to open {}. Check permissions (e.g., dialout group).", device_path);
        return false;
    }

    // Configure the serial port natively to prevent board resets
    struct termios tty;
    if (tcgetattr(fd, &tty) != 0) {
        my_logger_g.logger->error("Error from tcgetattr on {}", device_path);
        close(fd);
        return false;
    }

    // Apply the raw-mode flags
    tty.c_cflag &= ~HUPCL; 
    tty.c_iflag &= ~(BRKINT | ICRNL | IMAXBEL);
    tty.c_oflag &= ~OPOST;
    tty.c_lflag &= ~(ISIG | ICANON | IEXTEN | ECHO | ECHOE | ECHOK | ECHOCTL | ECHOKE);

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        my_logger_g.logger->error("Error from tcsetattr on {}", device_path);
        close(fd);
        return false;
    }

    // Append newline and write the payload
    std::string payload = cmd + "\n"; 
    ssize_t bytes_written = write(fd, payload.c_str(), payload.length());
    
    close(fd);

    // Verify the write was entirely successful
    if (bytes_written < 0 || static_cast<size_t>(bytes_written) != payload.length()) {
        my_logger_g.logger->error("Failed to write to {}. Wrote {} bytes.", device_path, bytes_written);
        return false;
    }

    my_logger_g.logger->debug("Successfully wrote command: {}", cmd);
    return true;
}

std::string execute_command_and_get_output(const std::string& program, const std::vector<std::string>& args, bool verbose) {
    my_logger_g.logger->debug("Executing command: {}", program);
    boost::filesystem::path full_path = boost::process::search_path(program);
    if (full_path.empty()) {
        full_path = program;
    }
    boost::process::ipstream out_stream;
    std::string result;
    std::string line;
    try {
        boost::process::child process(full_path, boost::process::args(args), boost::process::std_out > out_stream);
        if (verbose) {
            std::cout << "\n--- Execution Output Start (" << program << ") ---\n";
        }
        while (std::getline(out_stream, line)) {
            result += line + "\n";   
            if (verbose) {
                std::cout << line << std::endl;
            }
        }
        if (verbose) {
            std::cout << "--- Execution Output End ---\n\n";
        }
        process.wait(); 
        int exit_code = process.exit_code();
        if (exit_code != 0) {
            my_logger_g.logger->warn("Command '{}' returned non-zero exit code {}. Output length: {} bytes.", 
                                     program, exit_code, result.length());
        } else {
            my_logger_g.logger->debug("Command '{}' executed successfully. Captured {} bytes.", 
                                      program, result.length());
        }
    } catch (const boost::process::process_error& ex) {
        my_logger_g.logger->error("OS failed to launch process '{}': {}", program, ex.what());
        throw std::runtime_error(std::string("execute_command_and_get_output failed: ") + ex.what());
    }
    return result;
}

} // namespace helpers
