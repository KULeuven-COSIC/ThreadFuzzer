#include "Protocol_Stack/OT_Instances/OT_TD.h"

#include "Communication/ot_pipe_helper.h"
#include "Communication/shm_layer_communication.h"
#include "Configs/Fuzzing_Settings/main_config.h"
#include "Configs/Fuzzing_Settings/technical_config.h"
#include "Configs/Fuzzing_Settings/timers_config.h"
#include "Protocol_Stack/OT_Instances/OT_types.h"
#include "helpers.h"
#include "my_logger.h"

#include <chrono>
#include <cstdlib>
#include <future>
#include <iostream>
#include <memory>
#include <pthread.h>
#include <sstream>
#include <string>
#include <sys/stat.h>

#include <boost/process.hpp>

extern Technical_Config technical_config_g;
extern Timers_Config timers_config_g;
extern My_Logger my_logger_g;
extern Main_Config main_config_g;
extern OT_Pipe_Helper ot_pipe_helper_g;

/**
   Hacky method for finding out if a async task is still running at a given
   point. borrowed from:
   https://stackoverflow.com/questions/10890242/get-the-status-of-a-stdfuture
 */
template <typename R> bool is_ready(std::future<R> const &f) {
  return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
}
template <typename R>
bool is_ready_after_millis(std::future<R> const &f, const int m) {
  return f.wait_for(std::chrono::milliseconds(m)) == std::future_status::ready;
}

/**
   TODO: is_running() and is_pid_alive() are now hardcoded to return true!
   -> turn these into a heartbeat method
 */
bool OT_TD::start() {
  if (ot_type_ == OT_TYPE::DUT) {
    sock_holder = ot_pipe_helper_g.dut_holder_;
    log_file_ = main_config_g.dut_log_file;
  } else {
    sock_holder = ot_pipe_helper_g.pg_holder_;
    log_file_ = main_config_g.gen_log_file;
  }
  return false;
}

/**
   Launches a node with given speed and given log-file and connects IO to
   existing pipes based on the ot_type supplied.
   TODO: include thread-safe conditionals to check the liveliness?
*/
bool OT_TD::start_node() {

  // std::cout << "- starting node " << static_cast<int>(ot_type) + 2
  //           << " with speed " << std::to_string(speed) << std::endl;

  std::filesystem::path dir_type = (ot_type_ == OT_TYPE::DUT) ? std::filesystem::path("DUT") : std::filesystem::path("PG");
  std::filesystem::path base_dir = my_logger_g.get_log_dir_path() / dir_type;
  if (!helpers::create_directories_if_not_exist(base_dir)) {
      std::cerr << "Could not create a directory: " << base_dir << std::endl;
      return false;
  }

  std::string timestamp = helpers::get_current_time_stamp_short();

  /* ASAN logging */
  std::filesystem::path asan_dir = base_dir / std::filesystem::path(main_config_g.asan_log_path);
  if (!helpers::create_directories_if_not_exist(asan_dir)) {
      std::cerr << "Could not create a directory: " << asan_dir << std::endl;
      return false;
  }
  asan_file_ = asan_dir / std::filesystem::path("asan_" + timestamp + ".log");

  /* Standard error logging */
  std::filesystem::path error_dir = base_dir / std::filesystem::path("error_logs");
  if (!helpers::create_directories_if_not_exist(error_dir)) {
      std::cerr << "Could not create a directory: " << error_dir << std::endl;
      return false;
  }
  error_file_ = error_dir / std::filesystem::path("error_" + timestamp + ".log");

  /* OpenThread logging */
  std::filesystem::path ot_log_dir = base_dir / std::filesystem::path("ot_logs");
  if (!helpers::create_directories_if_not_exist(ot_log_dir)) {
      std::cerr << "Could not create a directory: " << ot_log_dir << std::endl;
      return false;
  }
  ot_log_file_ = ot_log_dir / std::filesystem::path("ot_log" + timestamp + ".log");


  /* format the command */
  int node_id = 2 + static_cast<int>(ot_type_);
  const std::string in_pipe = "in_" + std::to_string(node_id) + ".pipe";
  const std::string out_pipe = "out_" + std::to_string(node_id) + ".pipe";

  if (process_) {
      if (process_->running()) {
          my_logger_g.logger->warn("Process is still running. Terminating it before starting a new one.");
          process_->terminate();
      }
  }

  process_ = std::make_unique<boost::process::child>(helpers::get_openthread_path_by_ot_type(ot_type_) +
      "/build/simulation/examples/apps/cli/" + get_name(), 
      boost::process::std_in < in_pipe,
      boost::process::std_out > out_pipe,
      boost::process::std_err > error_file_.string(),
      boost::process::env["ASAN_OPTIONS"] = "log_path='" + asan_file_.string() + "'",
      boost::process::args={
        "-s", std::to_string(timers_config_g.speed),
        "-l", ot_log_file_.string(),
        std::to_string(node_id)
  });

  if (process_) {
    asan_file_ += "." + std::to_string(process_->id()); /* Update ASAN file name. */
  }

  if (!process_) {
      my_logger_g.logger->error("Failed to start a child process");
      return false;
  }

  if (!process_->running()) {
      my_logger_g.logger->error("Process is not running! Exit code: {}", process_->exit_code());
      return false;
  }

  my_logger_g.logger->info("Started the child process: {} in thread: {}", process_->id(), std::this_thread::get_id());
  return true;
}

/* NOTE: quite aggressive! Kills the node outright! */
bool OT_TD::stop() {

  if (process_) {
    my_logger_g.logger->info("Terminating the child process: {} from thread: {}", process_->id(), std::this_thread::get_id());
    process_->terminate();
  }

  my_logger_g.logger->info("Clean up the logs after stop");
  log_clean_up();

  return true;
}

// NOTE: we now use this to restart each time, therefore
// no distinction needed
bool OT_TD::reset() {
  if (!this->activate_thread()) {
    my_logger_g.logger->error("Failed to activate thread in " + session_name_);
    return false;
  }
  return true;
}

bool OT_TD::is_running() {
  if (!process_->running()) {
    auto exitcode = process_->exit_code();
    my_logger_g.logger->info("Node is not running. Exitcode: {}", exitcode);
    my_logger_g.logger->info("Cleaning up the logs");
    log_clean_up();
    return false;
  }
  return true;
}

bool OT_TD::restart() {

  node_id_ = static_cast<int>(ot_type_) + 2;

  /* set the right log file TODO: move this */
  std::cerr << "RESET of the " << session_name_ << std::endl;
  my_logger_g.logger->debug("RESET OF " + session_name_);

  /* first iteration? start a node thread */
  std::cout << "NODE IS FOUND DEAD, RESTARTING" << std::endl;

  /* Close previously opened pipes. */
  ot_pipe_helper_g.close_pipes(ot_type_);

  /* create the named pipes for node stdin and stdout */
  mkfifo(("in_" + std::to_string(node_id_) + ".pipe").c_str(), 0666);
  mkfifo(("out_" + std::to_string(node_id_) + ".pipe").c_str(), 0666);

  auto node_thread = std::async(&OT_TD::start_node, this); 

  /* now connect pipes to node I/O */
  if (!ot_pipe_helper_g.open_pipes(ot_type_, node_id_)) {
    my_logger_g.logger->error("failed to open pipes for node {} ", node_id_);
    std::cout << "failed to open pipes" << std::endl;
    return false;
  }

  /* Wait for the result of the initialization */
  if (!node_thread.get()) {
      my_logger_g.logger->error("Failed to start a node");
      return false;
  }

  return true;
}

bool OT_TD::activate_thread() {
  // std::cout << "THREAD activating on " << session_name_ << std::endl;
  my_logger_g.logger->info("activating thread... on {}", session_name_);

  std::string templte;
  if (ot_type_ == OT_TYPE::DUT)
    templte = main_config_g.dut_cli_args.back() + "\r\nDone";
  else
    templte = main_config_g.pg_cli_args.back() + "\r\nDone";

  if (!factoryreset()) {
    my_logger_g.logger->warn("Factory reset failed!");
    return false;
  }

  // if (!ot_pipe_helper_g.node_send_simple(ot_type_, "factoryreset\n"))
  //   return false;

  // std::this_thread::sleep_for(std::chrono::milliseconds(10));

  auto future_output_lines = ot_pipe_helper_g.node_async_reader(
      ot_type_, timers_config_g.node_async_reader_timeout_ms, templte); /* NOTE: very important */

  if (!ot_pipe_helper_g.node_send_simple(
          ot_type_,
          "dataset set active " + technical_config_g.network_dataset + "\n")) {
    my_logger_g.logger->error("Failed to set a dataset in " + session_name_);
    std::cout << "failed to set dataset" << std::endl;
  }

  // NOTE std::this_thread::sleep_for(std::chrono::microseconds(200));

  // next run additional commands from a the global config file
  // depending on who we are
  std::vector<std::string> cli_commands = {};
  if (ot_type_ == OT_TYPE::DUT) {
    cli_commands = main_config_g.dut_cli_args;
  } else {
    cli_commands = main_config_g.pg_cli_args;
  }

  for (auto cmd : cli_commands) {
    if (!ot_pipe_helper_g.node_send_simple(ot_type_, cmd + "\n")) {
      std::cout << "could not set the commands" << std::endl;
      return false;
    }

    // NOTE: std::this_thread::sleep_for(std::chrono::microseconds(50));
  }

  // next collect the results
  my_logger_g.logger->info("worker is {}", is_ready(future_output_lines));
  auto lines = future_output_lines.get();

  // std::cout << "worker found " << lines.size() << " lines" << std::endl;
  my_logger_g.logger->info("worker found {} lines, expected {}", lines.size(),
                           cli_commands.size() * 2 + 2);
  // std::cout << lines << std::endl;

  if (lines.size() < cli_commands.size() * 2 + 2) {
    my_logger_g.logger->error(
        "amount of lines does not correspond with {}!! It is {}",
        cli_commands.size() * 2 + 2, lines.size());
    for (auto line : lines)
      my_logger_g.logger->error("> \"{}\"", line);
    std::cout << "AMOUNT OF LINES FOUND BY WORKER IS WRONG!! IT IS "
              << lines.size() << " NOT " << cli_commands.size() * 2 + 2
              << std::endl;
    return false;
  }

  // next check these results
  for (auto line : lines) {
    if (line.find("Invalid") != std::string::npos ||
        line.find("Error") != std::string::npos) {
      std::cout << "TRAGIC FAILURE TO CONFIG THE NODE!" << std::endl;
      my_logger_g.logger->error("TRAGIC FAILURE TO CONFIG THE NODE!");
      // break;
      for (auto line_n : lines)
        my_logger_g.logger->info("broken {} line: {}",
                                 helpers::get_name_prefix_by_ot_type(ot_type_),
                                 line_n);
      return false;
    }
  }

  // for (auto line : lines)
  //   my_logger_g.logger->info(
  //       "{} line: {}", helpers::get_name_prefix_by_ot_type(ot_type_), line);

  return true;
}

bool OT_TD::deactivate_thread() {

  // if (helpers::stuff_cmd_to_screen(session_name_, "state")) {    // if
  // (helpers::stuff_cmd_to_screen(session_name_, "factoryreset")) {
  //     my_logger_g.logger->error("Failed to run \"factoryreset\"");
  //     return false;
  // }

  //     my_logger_g.logger->error("Failed to query " + session_name_ + "'s
  //     state"); return false;
  // }

  // if (helpers::stuff_cmd_to_screen(session_name_, "thread stop")) {
  //     my_logger_g.logger->error("Failed to stop a Thread in " +
  //     session_name_); return false;
  // }

  // if (helpers::stuff_cmd_to_screen(session_name_, "ifconfig down")) {
  //     my_logger_g.logger->error("Failed to run \"ifconfig down\"");
  //     return false;
  // }

  return true;
}

/**
   Factory reset a virtual node.
   NOTE: takes some time, as it return after successful prompt or a set delay.
   TODO: don't hard-code the max delay.
*/
bool OT_TD::factoryreset() {
  auto future_output_lines = ot_pipe_helper_g.node_async_reader(
      ot_type_, timers_config_g.node_async_reader_timeout_ms, "> "); // NOTE: very important

  if (!ot_pipe_helper_g.node_send_simple(ot_type_, "factoryreset\n")) {
    std::cout << "could not execute factory reset on " << session_name_
              << std::endl;
    my_logger_g.logger->warn("{} failed on {}", "factoryreset",
                             helpers::get_name_prefix_by_ot_type(ot_type_));
    return false;
  }

  auto lines = future_output_lines.get();

  for (auto line : lines) {
    my_logger_g.logger->debug("fr: \"{}\"", line);
  }

  if (lines.size() > 2 || lines.size() == 0) {
    my_logger_g.logger->warn("we got a wrong amount of lines: {}", lines.size());
    return false;
  }

  my_logger_g.logger->info("fr success!!");

  return true;
}

void OT_TD::log_clean_up() {
  if (!helpers::set_permissions_if_path_exists(error_file_, std::filesystem::perms::all)) {
      my_logger_g.logger->warn("Failed to set permissions to the {}", error_file_);
  }
  if (!helpers::set_permissions_if_path_exists(asan_file_, std::filesystem::perms::all)) {
      my_logger_g.logger->warn("Failed to set permissions to the {}", asan_file_);
  }
  if (!helpers::set_permissions_if_path_exists(ot_log_file_, std::filesystem::perms::all)) {
      my_logger_g.logger->warn("Failed to set permissions to the {}", ot_log_file_);
  }

  helpers::delete_if_file_is_empty(error_file_); /* Avoid keeping empty files */
}