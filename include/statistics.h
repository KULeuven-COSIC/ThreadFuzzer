#pragma once

#include <cstdlib>
#include <iostream>
#include <string>

struct Statistics {
  size_t hang_counter = 0;
  size_t crash_counter = 0;
  size_t empty_iterations = 0;

  size_t rand_mutator_counter = 0;
  size_t min_mutator_counter = 0;
  size_t max_mutator_counter = 0;
  size_t add_mutator_counter = 0;
  size_t sub_mutator_counter = 0;
  size_t injection_counter = 0;
  size_t len_mutation_counter = 0;

  size_t failed_fuzz_iterations_counter = 0;
  bool has_this_iteration_failed = false;

  size_t protocol_stack_crash_counter = 0;
  size_t dut_crash_counter = 0;

  size_t dut_become_router_counter = 0;
  double avg_iteration_time_s = 0.0;

  size_t long_silence = 0;
  size_t epochs = 0.0;
  size_t it_in_epochs = 0.0;
  size_t refinement_runs = 0.0;
  size_t refinements = 0.0;

  size_t dut_reboot_counter = 0.0;

  size_t nb_of_cid_responses = 0;
};
