#include "Fuzzers/reboot_cnt_fuzzer.h"

#include "Configs/Fuzzing_Strategies/fuzz_strategy_config.h"
#include "patch.h"
#include "statistics.h"

#include "my_logger.h"

#include <algorithm>
#include <memory>
#include <thread>

extern Fuzz_Strategy_Config fuzz_strategy_config_g;
extern My_Logger my_logger_g;
extern Statistics statistics_g;

// TODO:
// perform a linear search -> refinement through the patches epoch by epoch
// when a hit occurs, perform binary search on the patches of a SINGLE
// refinement iteration
// In essence, we should only be using the tried_patches for binary search.
// The reason our method works right now, is because the target patch is
// probably somewhere around the start of the saved_patches, as it was triggered
// during the previous refinement. This would result in the left leg of the
// search tree. Therefore, we are correctly focusing our effort on the target,
// getting down to the number of patches of a single iteration. However, using
// binary search only for tried_patches that yield a hit, would probably take
// the total number of refinements down with a few iterations.
//
// Especially for large epoch-sizes or big mutation rates, this number of saved
// iterations could snowball, given our low throughput.

// TODO 2:
// at a trigger, before refinement starts, check whether known crashing patches
// (in a crash_patches vector) occur in the saved_patches. Only if they don't
// occur or if the number of spurrious reboots is higher than the number of
// occurring crash_patches, perform the refinement.
// This also need modifications of the reboot check to account for the
// crash-patches occurring. The search should not be guided towards these cases!



bool RebootCntFuzzer::init() {
  std::cout << "RBT CNT FUZZING!! " << std::endl;
  
  ring_dinnerbell();
  // create the fuzzer we want to call during the epochs
  it_cnt = 0;
  refinement = false;
  // first_epoch = true;
  // epoch_it = false; // start with a reboot-cnt
  if (fuzz_strategy_config_g.chip_recommissioning_step) {
    // automatic factory-reset for Nanoleaf lightbulb
    if (fuzz_strategy_config_g.chip_device_name == "NANOLEAF" || fuzz_strategy_config_g.chip_device_name == "EVE") {
      std::cout << "F-RESET THE DUT" << std::endl;
      // NOTE: problem... reset is not controllable from here?
      // so reset has to be performed somewhere else...
      // TODO: need to do the same for factoryreset
    } else {
      // all other devices, let user perform the factory-reset
      std::cout << "PRE-INIT step, make sure to put device in pairing mode..."
                << std::endl;
      std::this_thread::sleep_for(std::chrono::seconds(30));
    }

    std::cout << "PAIRING the device using CHIP" << std::endl;
    helpers::chip_pair(fuzz_strategy_config_g.chip_device_name);

    current_state = State::NORMAL; /* no need to fetch the initial count, it will be zero anyway */

  } else {
    current_state = State::INIT;
  }
  return RandomFuzzer::init();
}

bool RebootCntFuzzer::fuzz(Packet &packet) {
  // are we at end of an epoch? disable fuzzing, as CHIP needs it
  if (current_state == State::EPOCH_IT || current_state == State::INIT || current_state == State::PRE_INIT) {
    my_logger_g.logger->info("[RBT_CNT_FUZZER]: PACKET WAS NOT FUZZED");
    return true;
  }
  // we are refining? only apply patches!
  if (current_state == State::REFINEMENT || fuzz_strategy_config_g.use_existing_seeds) {
    my_logger_g.logger->info("[RBT_CNT_FUZZER]: TRYING REFINEMENT...");
    // TODO: try using "iteration" variable in patch to make sure they are only
    // applied to the first packet they can fit on!!
    return apply_predefined_patches(packet);
  }
  // only thing left is state == NORMAL
  return RandomFuzzer::fuzz(packet);
  // return true;
}

bool RebootCntFuzzer::prepare_new_iteration() {
  // it_cnt++;
  statistics_g.it_in_epochs = it_cnt;

  switch (current_state) {

  case State::INIT: {
    /* FIRST ITERATION: FETCH INITIAL REBOOT-CNT */
    my_logger_g.logger->info("[RBTCNT_FUZZER]: INIT");
    std::cerr << "Checking initial reboot-count on the device via CHIP..."
              << std::endl;
    int current_reboot_count = chip_check_diagnostics();
    reboot_count = current_reboot_count;
    statistics_g.dut_reboot_counter = current_reboot_count;
    my_logger_g.logger->info("INITIAL REBOOTS: {}", current_reboot_count);
    current_state = State::NORMAL;
    break;
  }

  case State::NORMAL: {
    /* NORMAL FUZZING BEHAVIOR, KEEP TRACK OF ITERATIONS IN EPOCH */
    // TODO: add the patches to the "patches" list
    my_logger_g.logger->info("[RBTCNT_FUZZER]: NORMAL");
    auto patches = current_seed->get_patches();
    my_logger_g.logger->info(
        "vvvvvvvvvvvvvvvvvvvvvvvvv SAVED PATCHES vvvvvvvvvvvvvvvvvvvvvvvvv");
    for (std::shared_ptr<Patch> patch : patches) {
      if (!patch->is_empty_patch()) {
        saved_patches.insert(patch);
        my_logger_g.logger->info("SAVED PATCH {}...", patch->get_id());
      }
    }
    my_logger_g.logger->info(
        "^^^^^^^^^^^^^^^^^^^^^^^^^ SAVED PATCHES ^^^^^^^^^^^^^^^^^^^^^^^^^");
    it_cnt++;
    if (it_cnt >= fuzz_strategy_config_g.epoch_size) {
      it_cnt = 0;
      current_state = State::EPOCH_IT;
    }
    // NOTE: quick fix to keep the prune bs in check
    left = true;

    break;
  }

  case State::EPOCH_IT: {
    /* EPOCH COMPLETE, CHECK THE REBOOT-COUNT  */
    statistics_g.epochs++;

    my_logger_g.logger->info("[RBTCNT_FUZZER]: EPOCH_IT");
    std::cerr << "Checking reboot-count on the device via CHIP..." << std::endl;
    int current_reboot_count = chip_check_diagnostics();
    bool spurrious_reboot =
        (current_reboot_count !=
          1 + fuzz_strategy_config_g.epoch_size); // || chip_check_diagnostics();

    // NOTE: reboot during epoch, from NORMAL to REFINEMENT
    if (!refinement && spurrious_reboot) {
      left = true;
      refinement = true;
      statistics_g.refinement_runs++;
      // sort the saved_patches for pruning in linear time!
      // if (!std::is_sorted(saved_patches.begin(), saved_patches.end())) {
      //   std::cout << "PATCHES ARE NOT SORTED!! SORTING NOW!" << std::endl;
      //   my_logger_g.logger->info(
      //       "[RBTCNT_FUZZER]: PATCHES ARE NOT SORTED! NOW SORTING");
      //   std::sort(saved_patches.begin(), saved_patches.end());
      // }
      // LEFT!!
      //predefined_patches = std::vector<std::shared_ptr<Patch>>(
      //    saved_patches.begin(),
      //    saved_patches.end() - saved_patches.size() / 2);
      predefined_patches = std::vector<std::shared_ptr<Patch>>(saved_patches.begin(), saved_patches.end());
      current_state = State::REFINEMENT;
      std::cout << "STARTING REFINEMENT!! " << std::endl;
      std::cout << "RBT CNT NOW " << current_reboot_count << std::endl;
      ring_dinnerbell();

      // NOTE: if this was merely a check -> return!!
      if (fuzz_strategy_config_g.use_existing_seeds) {
        std::cout << "CRASH CHECK SUCCESFUL!" << std::endl;
        statistics_g.dut_crash_counter++;
        return false;
      }
    }

    // NOTE: refinement did not work, try other patches
    else if (refinement && !spurrious_reboot && saved_patches.size()) {
      // TODO check if predefined patches has size 1, then take the next one from saved_patches
      // TODO the problem, if no saved patches -> FAIL!!

      if (predefined_patches.size() == 1 && saved_patches.size() > 1) {
        // first erase the first element from the saved ones
        saved_patches.erase(tried_patches.begin());
        // next get the next predefined patch
        // hoping that by now, the saved_patches will already contain the
        // previously found tried_patches!
        predefined_patches.clear();
        predefined_patches.push_back(*(saved_patches.begin()));

      } else if (saved_patches.size() > 2) {

        std::cout << "NOTHING FOUND! PRUNING " << tried_patches.size()
                  << " patches!!!" << std::endl;
        my_logger_g.logger->info("PRUNING THE RIGHT SIDE!");

        // prune the tried patches
        prune_saved_patches();
        predefined_patches = std::vector<std::shared_ptr<Patch>>(
            saved_patches.begin(), saved_patches.end());
        // switch the leg
        // switch_bs_leg();

        // } else if (saved_patches.size() == 2) {
        //   std::cout << "GOT TO 2 PATCHES LEFT!!" << std::endl;
        //   // RIGHT one is culprit, only two elements
        //   predefined_patches = {saved_patches[1]};
        //   saved_patches = {saved_patches[1]};
      } else { // TODO fix this!! no use in trying this again!!!
        std::cout << "REFINEMENT END-CASE?!!" << std::endl;
        predefined_patches = std::vector<std::shared_ptr<Patch>>(
            saved_patches.begin(), saved_patches.end());
        saved_patches.clear();
      }
      current_state = State::REFINEMENT;
      std::cout << "REFINEMENT YIELDED NO RESULT!! SWITCHING PATCH"
                << std::endl;
      std::cout << "-> STILL " << saved_patches.size() << " patches left "
                << std::endl;

      // draw_saved_patches();
      // tried_patches.clear();
      // ring_dinnerbell();
    }
    // NOTE: crash during refinement!
    else if (refinement && spurrious_reboot) {
      std::cout << "RBT CNT WAS " << reboot_count << " NOW "
                << current_reboot_count << std::endl;

      // NOTE: REFINEMENT OR NORMAL & ~RBT => NORMAL
      if (tried_patches.size() == 1) {
        // search complete
        std::cout << "CRASH FOUND DURING REFINEMENT!!" << std::endl;
        std::cout << "RESUMING NORMAL EXECUTION" << std::endl;

        my_logger_g.logger->info(
            "[RBTCNT_FUZZER]: FOUND CRASH DUE TO THIS PATCH!!");
        my_logger_g.logger->info("{}", *(tried_patches.begin()->get()));
        statistics_g.dut_crash_counter++;
        predefined_patches.clear();
        saved_patches.clear();
        refinement = false;
        draw_size = 0;
        current_state = State::NORMAL;
      } else {
        // NOTE: removed this speed-up for now
        // search not complete, keep only this half
        //if (tried_patches.size() < predefined_patches.size()) {
        //  saved_patches = tried_patches;
        //} else {
        saved_patches = tried_patches;
        // }
        predefined_patches.clear();
        predefined_patches.push_back(*(tried_patches.begin()));
        //std::vector<std::shared_ptr<Patch>>(
        //    saved_patches.begin(),
        //    saved_patches.end() - saved_patches.size() / 2);
        std::cout << "SEARCH SUCCES..." << std::endl;
        std::cout << "NARROWING DOWN TO " << saved_patches.size() << std::endl;
        current_state = State::REFINEMENT;
      }

      // tried_patches.clear();

      // draw_saved_patches();

      ring_dinnerbell();

    }
    // NOTE: no patches left, so refinement failed!!
    else if (!saved_patches.size() && refinement) {
      // if (spurrious_reboot && saved_patches.size() == 1) {
      //   std::cout << "CRASH FOUND AT THE CUSP!!" << std::endl;
      //   my_logger_g.logger->info(
      //       "[RBTCNT_FUZZER]: FOUND CRASH DUE TO THIS PATCH!!");
      //   log_patch(predefined_patches[0]);
      // }
      my_logger_g.logger->info("[RBTCNT_FUZZER]: REFINEMENT FAILED!");
      std::cout << "REFINEMENT FAILED!!" << std::endl;
      predefined_patches.clear();
      saved_patches.clear();
      // tried_patches.clear();
      refinement = false;
      draw_size = 0;
      current_state = State::NORMAL;
      std::fill(drawn_patch_list.begin(), drawn_patch_list.end(), 0);
      to_draw_patch_list.clear();

    } else {
      my_logger_g.logger->info("NO BAD REBOOT");
      std::cout << "NO BAD REBOOT!!" << std::endl;
      // predefined_patches.clear();
      saved_patches.clear();
      // refinement = false;
      draw_size = 0;
      current_state = State::NORMAL;
    }

    reboot_count = current_reboot_count;
    statistics_g.dut_reboot_counter = current_reboot_count;

    my_logger_g.logger->info("REBOOTS: {}", current_reboot_count);

    // draw_saved_patches();
    tried_patches.clear();

    break;
  }

  case State::REFINEMENT: {
    /* REFINE THE NB OF FIELDS TO FUZZ */
    my_logger_g.logger->info("[RBTCNT_FUZZER]: REFINEMENT");
    std::cout << "REFINEMENT EPOCH IT: just tried: "
              << tried_patches.size() << "/" << saved_patches.size()
              << " patches " << std::endl;
    // NOTE: a refinement step is only one iteration, therefore,
    //go back to the epoch_it check again after one!
    std::cout << "PATCHES APPLIED DURING REFINEMENT: ";
    my_logger_g.logger->info("[RBTCNT_FUZZER]: PATCHES");
    for (auto patch : tried_patches) {
      if (!patch->is_empty_patch()) {
        std::cout << patch->get_id() << ", ";
        my_logger_g.logger->info("PATCH {}", patch->get_id());
      } else {
        std::cout << "... patch was empty!!! ..." << std::endl;
      }
    }
    std::cout << std::endl;

    // draw_saved_patches();

    current_state = State::EPOCH_IT;
    break;
  }

  case State::PRE_INIT: {
    // How to check whether the check was succesful?
    std::cout << "Let's hope that commissioning was succesful" << std::endl;
    current_state = State::NORMAL;
  }

  default:
    break;
  }

  return RandomFuzzer::prepare_new_iteration();
}

int RebootCntFuzzer::chip_check_diagnostics() {
  std::cout << "fetching rbt cnt..." << std::endl;
  // int current_reboot_count = std::atoi(
  //       ask_chip("/home/jakob/Documents/uni/doc/project/connectedhomeip/"
  //                "OWN_BUILD_DIR/chip-tool generaldiagnostics read "
  //                "active-network-faults 6 0 | grep -o \"ActiveNetworkFaults:
  //                .*\"")
  //           .substr(21)
  //           .c_str());
  int current_reboot_count = std::stoi(
      ask_chip("/home/jakob/Documents/uni/doc/project/connectedhomeip/"
               "OWN_BUILD_DIR/chip-tool generaldiagnostics read "
               "reboot-count 6 0 | grep -o \"RebootCount: .*\"")
          .substr(13)
          .c_str());
  std::cout << "diag count: " << std::to_string(current_reboot_count) << std::endl;
  my_logger_g.logger->info("COLLECTED RBT CNT: {}", current_reboot_count);
  return current_reboot_count;

}

std::string RebootCntFuzzer::ask_chip(const char *cmd) {
  std::array<char, 128> buffer;
  std::string result;
  std::unique_ptr<FILE, void (*)(FILE *)> pipe(popen(cmd, "r"),
                                               [](FILE *f) -> void {
                                                 // wrapper to ignore the return
                                                 // value from pclose() is
                                                 // needed with newer versions
                                                 // of gnu g++
                                                 std::ignore = pclose(f);
                                               });

  if (!pipe) {
    throw std::runtime_error("popen() failed!");
  }
  while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe.get()) !=
         nullptr) {
    result += buffer.data();
  }
  return result;
}

void RebootCntFuzzer::ring_dinnerbell() {
  std::cout << "\a" << std::endl;
  // std::cout << std::endl;
  // std::this_thread::sleep_for(std::chrono::milliseconds(200));
  // std::cout << "\a" << std::endl;
  // std::cout << std::endl;
  // std::this_thread::sleep_for(std::chrono::milliseconds(200));
  // std::cout << "\a" << std::endl;
  // std::cout << std::endl;
  // std::this_thread::sleep_for(std::chrono::milliseconds(200));
}

void RebootCntFuzzer::prune_saved_patches() {
  std::set<std::shared_ptr<Patch>> diff = {};
  // take difference with patches tried
  // std::set_difference(saved_patches.begin(), saved_patches.end(),
  //                    tried_patches.begin(), tried_patches.end(),
  //                    std::inserter(diff, diff.begin()));
  //saved_patches = diff;

  my_logger_g.logger->info("--- PRUNING: ---");
  std::cout << "PRUNING patch ids: ";
  for (auto saved_patch : saved_patches) {
    bool match = false;
    for (auto tried_patch : tried_patches) {
      if (tried_patch == saved_patch) {
        match = true;
        break;
      }
    }
    if (!match) {
      diff.insert(saved_patch);
    } else {
      std::cout << saved_patch->get_id() << ", ";
      my_logger_g.logger->info("id: {}", saved_patch->get_id());
    }
  }
  std::cout << std::endl;

  saved_patches = diff;

}

void RebootCntFuzzer::switch_bs_leg() {
  // if (left) {
  //   // we tried left before, switch to right
  //   predefined_patches = std::vector<std::shared_ptr<Patch>>(
  //       saved_patches.begin() + saved_patches.size() / 2, saved_patches.end());
  //   left = false;
  // } else {
  //   // we tried right before switch to left
  //   predefined_patches = std::vector<std::shared_ptr<Patch>>(
  //       saved_patches.begin(), saved_patches.end() - saved_patches.size() / 2);
  //   left = true;
  // }
}

// NOTE: only overridden to be able to log the patches applied
bool RebootCntFuzzer::apply_predefined_patches(Packet &packet) {
  if (!predefined_patches.empty()) {
    // TODO: figure out why "equality" is here so verbose?
    // -> might just be because we match patch on packet?
    auto it = std::find_if(
        predefined_patches.begin(), predefined_patches.end(),
        [&](const std::shared_ptr<Patch> &patch) {
          // Packet - Patch matching based on
          // packet summary (long and short) and packet layer
          return ((patch->get_packet_summary_short() ==
                       packet.get_summary_short() &&
                   !packet.get_summary_short().empty()) ||
                  (patch->get_packet_summary() == packet.get_summary() &&
                   !packet.get_summary().empty())) &&
                 (patch->get_layer() == packet.get_layer());
        });
    if (it != predefined_patches.end()) {
      std::shared_ptr<Patch> &patch = *it;
      my_logger_g.logger->info("Applying predefined patch: {}", *(patch.get()));
      if (!patch->apply(&packet)) {
        my_logger_g.logger->debug("Couldn't apply patch {}", patch->get_id());
        return false;
      }
      tried_patches.insert(patch); // NOTE: our only modification!!
      log_patch(patch);
      return true;
    }
    my_logger_g.logger->debug("No predefined patches to the current packet");
  }
  my_logger_g.logger->debug("No predefined patches to apply");
  return false;
};

void RebootCntFuzzer::draw_saved_patches() {

  if (saved_patches.size() > draw_size) {
    draw_size = saved_patches.size();
    // fill the index list
    for (auto p : saved_patches) {
      to_draw_patch_list.push_back(p->get_id());
    }
  }

  std::cout << "SAVED_SIZE:" << saved_patches.size() << std::endl;

  std::cout << "TRIED_PATCHES_MAP: |";
  int di = 0;
  for (auto i : to_draw_patch_list) {
    bool filled = false;
    if (drawn_patch_list[di]) {
      std::cout << "█";
      filled = true;
    } else {
      for (auto it : tried_patches) {
        if (i == it->get_id()) {
          std::cout << "#";
          drawn_patch_list[di] = 1;
          filled = true;
          break;
        }
      }
    }
    if (!filled) {
      std::cout << "░";
    }
    di++;
  }
  std::cout << "|" << std::endl;
}
