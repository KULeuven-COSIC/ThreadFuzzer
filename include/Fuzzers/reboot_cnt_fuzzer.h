#pragma once

#include "Fuzzers/random_fuzzer.h"
#include "packet.h"
#include "patch.h"

#include <memory>
#include <string>
#include <vector>
#include <set>

class RebootCntFuzzer : public RandomFuzzer {
public:
  virtual ~RebootCntFuzzer() {}
  virtual bool init() override;
  virtual bool fuzz(Packet &packet) override;
  virtual bool prepare_new_iteration() override;
  virtual bool apply_predefined_patches(Packet &packet) override;

private:
  enum State {
    PRE_INIT,
    INIT,
    NORMAL,
    EPOCH_IT,
    REFINEMENT
  };

  bool refinement;

  State current_state;

  //std::set<std::string> epoch_field_set;
  int it_cnt;
  bool first_epoch;
  bool epoch_it;
  int reboot_count;

  std::set<std::shared_ptr<Patch>> saved_patches;
  std::set<std::shared_ptr<Patch>> tried_patches;

  bool left;

  int chip_check_diagnostics();

  std::string ask_chip(const char * cmd);

  void ring_dinnerbell();

  void prune_saved_patches();
  void switch_bs_leg();

  void draw_saved_patches();
  std::vector<unsigned long int> to_draw_patch_list;
  std::vector<unsigned long int> drawn_patch_list = std::vector<unsigned long int>(300);
  long unsigned int draw_size = 0;
  };
