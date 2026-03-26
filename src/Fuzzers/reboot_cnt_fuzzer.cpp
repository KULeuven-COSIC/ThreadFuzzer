#include "Fuzzers/reboot_cnt_fuzzer.h"

#include "Configs/Fuzzing_Strategies/fuzz_strategy_config.h"
#include "Fuzzers/base_fuzzer.h"
#include "helpers.h"
#include "mutation.h"
#include "patch.h"
#include "statistics.h"

#include "DUT/DUT_factory.h"
#include "DUT/DUT_names.h"

#include "my_logger.h"

#include <algorithm>
#include <any>
#include <memory>
#include <thread>
#include <vector>

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
    if (main_config_g.dut_name == DUT_NAME::NANOLEAF ||
        main_config_g.dut_name == DUT_NAME::EVE_SENSOR) {
      std::cout << "THE DUT SHOULD BE F_RESET HERE" << std::endl;
      // NOTE: problem... reset is not controllable from here?
      // so reset has to be performed somewhere else...
      // TODO: need to do the same for factoryreset
    } else {
      // all other devices, let user perform the factory-reset
      std::cout << "PRE-INIT step, make sure to put device in pairing mode..."
                << std::endl;
      std::this_thread::sleep_for(std::chrono::seconds(3));
    }

    std::cout << "PAIRING the device using CHIP" << std::endl;
    /* crash if chip_pair fails */
    if (helpers::chip_pair())
      return false;

    reboot_count = helpers::chip_check_diagnostics();
    statistics_g.dut_reboot_counter = reboot_count;
    std::cout << "FIRST RBT CNT: " << reboot_count << std::endl;

    current_state = State::NORMAL; /* no need to fetch the initial count, it
                                      will be zero anyway */

  } else {
    current_state = State::INIT;
  }
  return RandomFuzzer::init();
}

bool RebootCntFuzzer::fuzz(Packet &packet) {
  // are we at end of an epoch? disable fuzzing, as CHIP needs it
  if (current_state == State::EPOCH_IT || current_state == State::INIT ||
      current_state == State::PRE_INIT || statistics_g.fuzz_lock) {
    my_logger_g.logger->info("[RBT_CNT_FUZZER]: PACKET WAS NOT FUZZED");
    return true;
  }
  // we are refining? only apply patches!
  if (current_state == State::REFINEMENT ||
      fuzz_strategy_config_g.use_existing_seeds) {
    my_logger_g.logger->info("[RBT_CNT_FUZZER]: TRYING REFINEMENT...");
    // TODO: try using "iteration" variable in patch to make sure they are only
    // applied to the first packet they can fit on!!
    return apply_predefined_patches(packet);
  }
  // only thing left is state == NORMAL
  return RandomFuzzer::fuzz(packet);
  // return true;
}

int RebootCntFuzzer::prepare_new_iteration() {
  // it_cnt++;
  statistics_g.it_in_epochs = it_cnt;

  bool hard_reset = false;

  // NOTE: very shady, bite me.
  reboot_count = statistics_g.dut_reboot_counter;

  switch (current_state) {

  case State::INIT: {
    /* FIRST ITERATION: FETCH INITIAL REBOOT-CNT */
    // my_logger_g.logger->info("[RBTCNT_FUZZER]: INIT");
    // std::cerr << "RE-pairing device via CHIP..." << std::endl;
    // my_logger_g.logger->warn(
    //     "re-pairing device via CHIP, this should not happen in the
    //     beginning!");
    // helpers::chip_pair();
    // int current_reboot_count = helpers::chip_check_diagnostics();
    // reboot_count = current_reboot_count;
    // statistics_g.dut_reboot_counter = current_reboot_count;
    // my_logger_g.logger->info("INITIAL REBOOTS: {}", current_reboot_count);
    // current_state = State::NORMAL;
    std::cout << "THIS STATE SHOULD NOT HAPPEN!!!" << std::endl;
    current_state = State::INIT;
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

    break;
  }

  case State::EPOCH_IT: {
    /* EPOCH COMPLETE, CHECK THE REBOOT-COUNT  */
    statistics_g.epochs++;

    my_logger_g.logger->info("[RBTCNT_FUZZER]: EPOCH_IT");
    std::cerr << "Checking reboot-count on the device via CHIP..." << std::endl;
    int current_reboot_count = helpers::chip_check_diagnostics();
    bool spurrious_reboot;
    if (refinement) {
      spurrious_reboot = (current_reboot_count - reboot_count) != 2;
    } else {
      spurrious_reboot =
          (current_reboot_count - reboot_count) !=
          (1 +
           fuzz_strategy_config_g.epoch_size); // || chip_check_diagnostics();
    }
    std::cout << "rbt_cnt diff: "
              << (current_reboot_count - reboot_count) -
                     (1 + fuzz_strategy_config_g.epoch_size)
              << std::endl;

    // NOTE: reboot during epoch, from NORMAL to REFINEMENT
    if (!refinement && spurrious_reboot) {
      hard_reset = true;
      ring_dinnerbell();
      // NOTE: if this was merely a check -> return!!
      if (fuzz_strategy_config_g.use_existing_seeds) {
        std::cout << "CRASH CHECK SUCCESFUL!" << std::endl;
        my_logger_g.logger->warn("crash found!");
        statistics_g.dut_crash_counter++;
        current_state = State::NORMAL;
        break;
      } else if (is_unique_crash() && fuzz_strategy_config_g.use_refinement) {
        statistics_g.refinement_runs++;
        refinement = true;
        predefined_patches = std::vector<std::shared_ptr<Patch>>(
            saved_patches.begin(), saved_patches.end());
        current_state = State::REFINEMENT;
        std::cout << "STARTING REFINEMENT!! ON " << saved_patches.size()
                  << " PATCHES" << std::endl;
        std::cout << "RBT CNT NOW " << current_reboot_count << std::endl;
      } else { // NOT unique crash
        std::cout << "CRASH: BUT PROLLY NOT UNIQUE" << std::endl;
        my_logger_g.logger->warn("crash but prolly not unique!");
        current_state = State::NORMAL;
        statistics_g.dut_nonunique_crash_counter++;
      }

    }

    // NOTE: refinement did not work, try other patches
    else if (refinement && !spurrious_reboot && saved_patches.size()) {
      // TODO check if predefined patches has size 1, then take the next one
      // from saved_patches
      // TODO the problem, if no saved patches -> FAIL!!
      // TODO: solution is quite convoluted, perhaps fix it?

      if (predefined_patches.size() == 1 && saved_patches.size() > 1) {
        // first erase the first element from the saved ones
        saved_patches.erase(saved_patches.begin());
        // next get the next predefined patch
        // hoping that by now, the saved_patches will already contain the
        // previously found tried_patches!
        predefined_patches.clear();
        // predefined_patches.push_back(*(saved_patches.begin()));
        predefined_patches.assign(saved_patches.begin(), saved_patches.end());

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
      hard_reset = true;
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
        saved_crashes.insert(*(tried_patches.begin()));
        my_logger_g.logger->info(
            "[RBTCNT_FUZZER]: FOUND CRASH DUE TO THIS PATCH!!");
        my_logger_g.logger->info("{}", *(tried_patches.begin()->get()));
        statistics_g.dut_crash_counter++;
        predefined_patches.clear();
        saved_patches.clear();
        refinement = false;
        draw_size = 0;
        current_state = State::NORMAL;
        hard_reset = true;
      } else {
        if (tried_patches.size() == saved_patches.size()) {
          std::cout << "SKIPPING THIS ONE: "
                    << (*(tried_patches.rbegin()))->get_id() << std::endl;
          tried_patches.erase(prev(tried_patches.end()));
          predefined_patches.clear();
          predefined_patches.assign(tried_patches.begin(), tried_patches.end());
          std::cout << "EDGE_CASE: ALL SAVED PATCHES ARE TRIED" << std::endl;
        } else {
          saved_patches = tried_patches;
          predefined_patches.clear();
          predefined_patches.assign(tried_patches.begin(), tried_patches.end());
          std::cout << "SEARCH SUCCES..." << std::endl;
          std::cout << "NARROWING DOWN TO " << saved_patches.size()
                    << std::endl;
        }
        current_state = State::REFINEMENT;
        hard_reset = true;
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
      hard_reset = true;

    } else {
      my_logger_g.logger->info("NO BAD REBOOT");
      std::cout << "NO BAD REBOOT!!" << std::endl;
      // predefined_patches.clear();
      saved_patches.clear();
      // refinement = false;
      draw_size = 0;
      current_state = State::NORMAL;
      hard_reset = true;
    }

    my_logger_g.logger->info("OLD REBOOTS: {}", reboot_count);
    reboot_count = current_reboot_count;
    // statistics_g.dut_reboot_counter = current_reboot_count;

    my_logger_g.logger->info("REBOOTS: {}", current_reboot_count);

    // draw_saved_patches();
    tried_patches.clear();

    break;
  }

  case State::REFINEMENT: {
    /* REFINE THE NB OF FIELDS TO FUZZ */
    my_logger_g.logger->info("[RBTCNT_FUZZER]: REFINEMENT");
    std::cout << "REFINEMENT EPOCH IT: just tried: " << tried_patches.size()
              << "/" << saved_patches.size() << " patches " << std::endl;
    // NOTE: a refinement step is only one iteration, therefore,
    // go back to the epoch_it check again after one!
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
    std::cout << "THIS STATE SHOULD NOT HAPPEN!!!" << std::endl;
    current_state = State::PRE_INIT;
  }

  default:
    break;
  }

  bool r = Base_Fuzzer::prepare_new_iteration();

  if (hard_reset && r) {
    my_logger_g.logger->info("[RBTCNT_FUZZER]: going back to init!");
    std::cout << "[RBTCNT_FUZZER]: going back to init!" << std::endl;
    return 2;
  }

  return r;
}

void RebootCntFuzzer::ring_dinnerbell() {
  std::cout << "\a" << std::endl;
  std::cout << std::endl;
  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  std::cout << "\a" << std::endl;
  std::cout << std::endl;
  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  std::cout << "\a" << std::endl;
  std::cout << std::endl;
  std::this_thread::sleep_for(std::chrono::milliseconds(200));
}

void RebootCntFuzzer::prune_saved_patches() {
  std::set<std::shared_ptr<Patch>> diff = {};
  // take difference with patches tried
  // std::set_difference(saved_patches.begin(), saved_patches.end(),
  //                    tried_patches.begin(), tried_patches.end(),
  //                    std::inserter(diff, diff.begin()));
  // saved_patches = diff;

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
  //       saved_patches.begin() + saved_patches.size() / 2,
  //       saved_patches.end());
  //   left = false;
  // } else {
  //   // we tried right before switch to left
  //   predefined_patches = std::vector<std::shared_ptr<Patch>>(
  //       saved_patches.begin(), saved_patches.end() - saved_patches.size() /
  //       2);
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

bool RebootCntFuzzer::mutation_contains(
    std::shared_ptr<Mutation> m1,
    std::vector<std::shared_ptr<Mutation>> mutations) {
  for (auto m2 : mutations) {
    if ((*m1).field->field_name == (*m2).field->field_name &&
        (*m1).field->index == (*m2).field->index) {
      std::cout << " > field name: " << (*m2).field->field_name
                << " vs: " << (*m1).field->field_name;
      std::cout << " > field name: " << (*m2).field->field_name
                << " vs: " << (*m1).field->field_name;
      return true;
    }
  }
  return false;
}

/**
   Check the saved_patches, see if they contain anything that resembles the
   crashes found before in saved_crashes.
*/
bool RebootCntFuzzer::is_unique_crash() {
  bool interesting = true;
  for (auto sp : saved_patches) {
    for (std::shared_ptr<Patch> cp : saved_crashes) {
      // TODO: check if command-type, field name and field value match!
      if (sp->get_packet_summary_short() == cp->get_packet_summary_short() &&
          std::any_of(sp->get_mutations().begin(), sp->get_mutations().end(),
                      [cp, this](std::shared_ptr<Mutation> m) {
                        return mutation_contains(m, cp->get_mutations());
                      })) {
        interesting = false;
        std::cout << "new patch: " << sp << " resembles saved patch: " << cp
                  << std::endl;
        my_logger_g.logger->info("new patch: {} resembles saved patch: {}", sp,
                                 cp);
        std::cout << "likely this crash is not unique" << std::endl;
        my_logger_g.logger->info("likely this crash is not unique");
      }
    }
  }
  if (!interesting) {
    std::cout << "this crash is likely unique!" << std::endl;
    my_logger_g.logger->info("this crash is likely unique!");
  }
  return interesting;
}
