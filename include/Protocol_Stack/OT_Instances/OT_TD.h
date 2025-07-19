#pragma once

#include <memory>
#include <string>

#include "DUT/OT_DUT.h"
#include "Protocol_Stack/Packet_Generator/OT_packet_generator.h"

#include "Communication/ot_pipe_helper.h"

#include <boost/process.hpp>

/* OpenThread Thread Device */
class OT_TD : public OT_Packet_Generator, public OT_DUT {
public:
  virtual ~OT_TD() = default;
  virtual bool start() override;
  virtual bool stop() override;
  virtual bool restart() override;
  virtual bool is_running() override;
  virtual bool reset() override;
  virtual bool activate_thread() override;
  virtual bool deactivate_thread() override;
  virtual bool factoryreset() override;

  virtual std::string get_name() const = 0;

protected:
  std::string session_name_;
  bool start_node();
  void log_clean_up();

  SocketHolder sock_holder;
  int node_id_ = -1;
  std::string log_file_;

  std::unique_ptr<boost::process::child> process_;
  // int pid_ = 0;

  std::filesystem::path ot_log_file_;
  std::filesystem::path asan_file_;
  std::filesystem::path error_file_;
};
