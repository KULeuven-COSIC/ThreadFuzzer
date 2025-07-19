#pragma once

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "packet.h"

#include "Configs/Fuzzing_Settings/main_config.h"
#include "Configs/Fuzzing_Strategies/fuzz_strategy_config.h"

#include "Protocol_Stack/OT_Instances/OT_types.h"

extern Fuzz_Strategy_Config fuzz_strategy_config_g;
extern Main_Config main_config_g;

/* DEFINE DIFFERENT COLORS */
#define BLACK "\033[0;30m"
#define RED "\033[0;31m"
#define GREEN "\033[0;32m"
#define YELLOW "\033[0;33m"
#define BLUE "\033[0;34m"
#define CRESET "\033[0m"

/* Overloading the << operator to print the vectors */
template <typename T>
inline std::ostream &operator<<(std::ostream &os, const std::vector<T> &v) {
  for (const T &t : v) {
    os << t << " ";
  }
  return os;
}

NLOHMANN_JSON_NAMESPACE_BEGIN
template <typename T> struct adl_serializer<std::shared_ptr<T>> {
  static void to_json(json &j, const std::shared_ptr<T> &ptr) { j = *ptr; }

  static void from_json(const json &j, std::shared_ptr<T> &ptr) {
    ptr = std::make_shared<T>(j.get<T>());
  }
};
NLOHMANN_JSON_NAMESPACE_END

namespace helpers {

std::string get_openthread_path_by_ot_type(OT_TYPE ot_type);
std::string get_name_prefix_by_ot_type(OT_TYPE ot_type);

std::string read_file_last_chars(const std::string &filename,
                                 int char_count = 4000);
int exec_system_cmd_with_timeout(const std::string &cmd, int timeout_s);
int exec_system_cmd_with_default_timeout(const std::string &cmd);
int create_screen_session(const std::string &screen_name,
                          const std::string &log_file_path = "");
int stop_screen_session(const std::string &screen_name);
int stuff_cmd_to_screen(const std::string &screen_name, const std::string &cmd);
int screen_session_exists(const std::string &screen_name);

int execute_screen_cmd(const std::string &cmd);

bool kill_process(const std::string &process_name);
bool kill_pid(const std::string &pid_name);
bool is_process_alive(const std::string &process_name);
bool signal_service(const std::string &service_name, const std::string &signal);
bool clear_instrumentation_files(const std::filesystem::path &path);
bool run_screen_cli_commands(const std::string session_name,
                             const std::vector<std::string> cli_commands);
bool is_pid_alive(const std::string &pid_name);
bool send_udp_heartbeat(int port);

bool send_udp_request(const std::string, int port);
bool run_udp_cli_commands(const std::vector<std::string> cli_commands,
                          int port);
int chip_pair(const std::string &device);
bool chip_unpair(const std::string &device);
std::string shell_command(const std::string cmd);

template <typename T>
inline bool vec_contains(const std::vector<T> &vec, const T &val) {
  return std::find(vec.begin(), vec.end(), val) != vec.end();
}

template <typename C>
inline bool
parse_json_file_into_class_instance(const std::filesystem::path &json_file_path,
                                    C &class_instance) {
  std::ifstream json_file(json_file_path);
  if (!json_file.is_open()) {
    std::cerr << "Could not open the file: " << json_file_path << std::endl;
    return false;
  }
  nlohmann::json json_obj;
  try {
    json_file >> json_obj;
    class_instance = json_obj.get<C>();
  } catch (std::exception &ex) {
    std::cerr << "Caught an exception during json parsing: " << ex.what()
              << std::endl;
    return false;
  }
  return true;
}

template <typename C>
inline bool
write_class_instance_to_json_file(C &class_instance,
                                  const std::filesystem::path &json_file_path) {
  std::ofstream json_file(json_file_path);
  if (!json_file.is_open()) {
    std::cerr << "Could not open the file: " << json_file_path << std::endl;
    return false;
  }
  try {
    nlohmann::json j = class_instance;
    json_file << std::setw(4) << j;
  } catch (std::exception &ex) {
    std::cerr << "Caught an exception during json loading: " << ex.what()
              << std::endl;
    return false;
  }
  return true;
}

bool delete_if_file_is_empty(const std::filesystem::path &path);
bool check_if_path_exists(const std::filesystem::path &path);
bool set_permissions_if_path_exists(const std::filesystem::path &path, std::filesystem::perms);
bool create_directories_if_not_exist(const std::filesystem::path &path);

inline std::string get_current_time_stamp() {
  time_t t = time(NULL);
  struct tm tm = *localtime(&t);
  char str[60];
  sprintf(str, "%d-%02d-%02d_%02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1,
          tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
  return std::string(str);
}

inline std::string get_current_time_stamp_short() {
  time_t t = time(NULL);
  struct tm tm = *localtime(&t);
  char str[60];
  sprintf(str, "%d-%02d-%02d_%02d-%02d-%02d", tm.tm_year + 1900, tm.tm_mon + 1,
          tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
  return std::string(str);
}

template <typename T> inline void random_shuffle(std::vector<T> &v) {
  static auto rng = std::default_random_engine{};
  std::ranges::shuffle(v, rng);
}

uint64_t EXP0(uint64_t limit,
              double lambda = 0.3); // Generate random number in range [0,
                                    // limit) following exponential distribution
double URD(double start = 0.0, double end = 1.0);
uint64_t UR(uint64_t start, uint64_t end); // // Generate random number in range [start, end)
uint64_t UR0(uint64_t limit); // Generate random number in range [0, limit)
uint64_t UR1(uint64_t limit); // Generate random number in range [1, limit]

std::string shorten_dissector_summary(const std::string &dissector_summary);

bool is_state_being_fuzzed(const std::string &state_name);
bool is_fuzzing_direction(uint8_t direction);

inline int clear_screen() { return system("clear"); }

const std::string get_layer_name_by_idx(uint8_t idx);
std::vector<std::string> get_field_prefixes_by_layer_idx(uint8_t mutex_name);
std::string get_dissector_by_layer_idx(uint8_t mutex_name);

inline Packet get_sample_packet() {

  uint8_t pdu[] = {0x41, 0xA3, 0x97, 0xE5, 0xA1, 0xF8};
  size_t pdu_size = 6;
  size_t layer_num = 0;

  auto p = Packet(pdu, pdu_size, layer_num, PACKET_SRC::SRC_DUT);
  p.set_dissector_name("lte_rrc.ul_ccch");

  return p;
}
} // namespace helpers
