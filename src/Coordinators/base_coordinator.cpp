#include "Coordinators/base_coordinator.h"

#include "statistics.h"
#include "my_logger.h"
#include "field.h"

#include "Fuzzers/base_fuzzer.h"
#include "Fuzzers/fuzzer_factory.h"
#include "Communication/shm_layer_communication_factory.h"
#include "Communication/shm_layer_communication.h"
#include "Configs/Fuzzing_Strategies/fuzz_strategy_config.h"
#include "Configs/Fuzzing_Settings/main_config.h"
#include "Protocol_Stack/protocol_stack_factory.h"
#include "DUT/DUT_factory.h"

#include "shm/fuzz_config.h"
#include "shm/shared_memory.h"

#include <nlohmann/json.hpp>

#include <stdlib.h>

extern My_Logger my_logger_g;
extern Main_Config main_config_g;
extern Fuzz_Strategy_Config fuzz_strategy_config_g;
extern Fuzz_Config fuzz_config_g;
extern Statistics statistics_g;

void to_json(nlohmann::json &j, const std::unordered_map<std::string, std::vector<Field> >& database) {
    for (auto& it : database) {
        j[it.first] = it.second;
    }
}
void from_json(const nlohmann::json &j, std::unordered_map<std::string, std::vector<Field> >& database) {
    for (auto& it : j.items()) {
        database[it.key()] = it.value().get<std::vector<Field> >();
    }
}

Base_Coordinator::Base_Coordinator()
{
    if (!helpers::create_directories_if_not_exist(hang_seeds_dir)) {
        my_logger_g.logger->error("Failed to create a directory: {}", hang_seeds_dir);
        throw std::runtime_error("Failed to create a directory!");
    }
    if (!helpers::create_directories_if_not_exist(crash_seeds_dir)) {
        my_logger_g.logger->error("Failed to create a directory: {}", crash_seeds_dir);
        throw std::runtime_error("Failed to create a directory!");
    }
    protocol_stack = Protocol_Stack_Factory::get_protocol_stack_by_protocol_name(main_config_g.protocol_stack_name);
    dut = DUT_Factory::get_dut_by_name(main_config_g.dut_name);
}

Base_Coordinator::~Base_Coordinator()
{
    protocol_stack->stop();
    dut->stop();
}

bool Base_Coordinator::init_fuzzing_strategies(const std::vector<std::string> &fuzz_strategy_config_names)
{
    fuzz_strategy_configs_.reserve(fuzz_strategy_config_names.size());
    for (int i = 0, i_max = fuzz_strategy_config_names.size(); i < i_max; ++i) {
        const std::string& fuzz_strategy_config_name = fuzz_strategy_config_names.at(i);
        Fuzz_Strategy_Config config;
        if (!helpers::parse_json_file_into_class_instance(fuzz_strategy_config_name, config)) {
            my_logger_g.logger->error("Failed to read config file: {}", fuzz_strategy_config_name);
            return false;
        }
        fuzz_strategy_configs_.push_back(std::move(config));
    }
    return true;
}

bool Base_Coordinator::renew_fuzzing_iteration()
{
    bool need_to_finish = false;
    bool need_to_restart_protocol_stack = false;

    /* 1. Check aliveliness (DUT and Protocol Stack) */
    if (!protocol_stack->is_running()) {
        std::string crash_seed_filename = crash_seeds_dir + "stack_crash_seed_";
        helpers::write_class_instance_to_json_file(Base_Fuzzer::current_seed, crash_seed_filename + helpers::get_current_time_stamp() + ".json");
        //iteration_result.was_crash_detected = true;
        need_to_restart_protocol_stack = true;
    }
    my_logger_g.logger->info("Completed fuzzing iteration: {} ({} for current config file)", global_iteration, local_iteration);

    /* 2. Update fuzzing "State" */
    global_iteration++;
    local_iteration++;
    wdissector_mutex.lock();

    /* Update coverage information and tweak probabilities */
    try {
        if (fuzz_strategy_config_g.use_coverage_logging) update_coverage_information();
        if (fuzz_strategy_config_g.use_probability_optimization) {
            update_probabilities(iteration_result.was_new_coverage_found);
        }
    } catch(...) {
        need_to_restart_protocol_stack = true;
    }

    my_logger_g.logger->debug("Number of mutated fields in this iteration: {}", Base_Fuzzer::mutated_fields_num);
    Base_Fuzzer::mutated_fields.clear();
    Base_Fuzzer::mutated_fields_num = 0;

    /* 3. Check if any of the fuzzers requests finish of the fuzzing */
    for (size_t i = 0; i < fuzzers.size(); i++) {
        bool need_to_finish_local = !fuzzers[i]->prepare_new_iteration();
        need_to_finish |= need_to_finish_local;
        if (need_to_finish_local) my_logger_g.logger->info("Fuzzer indexed {} requested finishing fuzzing", i);
    }

    if (!need_to_restart_protocol_stack) {
      //  try {
      //      if (!reset_target()) need_to_restart_protocol_stack = true; // TODO: Why is it needed?
      //  } catch(...) {
      //      throw;
      //  }
    } else wdissector_mutex.unlock();

    if (statistics_g.has_this_iteration_failed) statistics_g.failed_fuzz_iterations_counter++;
    helpers::clear_screen(); print_statistics();
    statistics_g.has_this_iteration_failed = false;

    need_to_finish |= (static_cast<int>(local_iteration) >= (fuzz_strategy_config_g.total_iterations == -1 ? INT_MAX : fuzz_strategy_config_g.total_iterations));
    if (need_to_finish) {
        my_logger_g.logger->info("Finishing fuzzing");
        my_logger_g.logger->debug("Current state machine:");
        my_logger_g.logger->flush();
    }

    my_logger_g.logger->flush();

    if (need_to_restart_protocol_stack)  {
        if (!protocol_stack->restart()) {
            my_logger_g.logger->error("Failed to restart the protocol stack!");
            return false;
        }
    }

    return !need_to_finish;
}

void Base_Coordinator::layer_fuzzing_loop(EnumMutex mutex_num) {

    const std::string layer_name = helpers::get_layer_name_by_idx(mutex_num);
    my_logger_g.logger->info("Starting {} thread", layer_name);
    std::unique_ptr<SHM_Layer_Communication> SHM_Comm = SHM_Layer_Communication_Factory::get_shm_layer_communication_instance_by_layer_num(mutex_num);
    my_logger_g.logger->info("Inited communication");
    
    std::string dissector = helpers::get_dissector_by_layer_idx(static_cast<int>(mutex_num));

    while (SHM_Layer_Communication::is_active) {
        bool failed = false;

        /* Recieve intercepted message */
        Packet pdu = SHM_Comm->receive();

        if (!pdu.get_size()) continue;

        pdu.set_dissector_name(dissector);

        wdissector_mutex.lock();
        my_logger_g.logger->info("[{}] Dissector {} {}", layer_name, dissector, pdu);
            
        if (pdu.get_packet_src() == PACKET_SRC::SRC_PROTOCOL_STACK)
        {
            if (!pdu.full_dissect()) {
                my_logger_g.logger->warn("[{}] Fuzz iteration failed in prepare_fuzz", layer_name);
                failed = true;
            }
            my_logger_g.logger->info("---> Dissector's summary: {}", pdu.get_summary());
            my_logger_g.logger->flush();

            bool is_state_fuzzed = helpers::is_state_being_fuzzed(pdu.get_summary());

            if (!failed && is_state_fuzzed) {
                for (size_t i = 0; i < fuzzers.size(); i++) {
                    if (!fuzzers.at(i)->fuzz(pdu)) {
                        my_logger_g.logger->warn("[{}] Fuzz iteration failed", layer_name);
                        failed = true;
                    }
                }
                
                std::string fuzzed_packet_type = "UNKNOWN";
                if (pdu.quick_dissect()) {
                    fuzzed_packet_type = pdu.get_summary_short();
                }
                my_logger_g.logger->info("Fuzzed packet: {} (type {})", pdu, fuzzed_packet_type);
            }
        } else {
            incoming_packet_num = incoming_packet_num + 1;
            if (pdu.quick_dissect()) {
                my_logger_g.logger->info("<--- Dissector's summary: {}", pdu.get_summary());
            } else {
                my_logger_g.logger->warn("Failed to dissect the packet! (dissector: {})", dissector);
            }

            /* Check if we want to stop after the reception of the current message */
            const std::string& packet_summary = pdu.get_summary();
            const std::string& packet_summary_short = pdu.get_summary_short();
            const std::vector<std::string>& stop_after_state_vec = fuzz_strategy_config_g.fuzzing_stop_states;
            if (!stop_fuzzing_iteration && (std::find(stop_after_state_vec.begin(), stop_after_state_vec.end(), packet_summary) != stop_after_state_vec.end()
                || std::find(stop_after_state_vec.begin(), stop_after_state_vec.end(), packet_summary_short) != stop_after_state_vec.end()))
            {
                my_logger_g.logger->info("Message \"{}\" triggered termination of the current fuzzing iteration", packet_summary);
                stop_fuzzing_iteration = true;
                statistics_g.dut_become_router_counter++;
            }
        }
        my_logger_g.logger->info("");
        my_logger_g.logger->flush();

        if (pdu.get_packet_src() == PACKET_SRC::SRC_PROTOCOL_STACK) {
            Base_Fuzzer::packet_buffer[pdu.get_dissector_name()].insert(pdu);
        }
        wdissector_mutex.unlock();
        SHM_Comm->send(pdu);
        if (failed) statistics_g.has_this_iteration_failed = true;
    }
}

void Base_Coordinator::terminate_fuzzing()
{
  std::cout << "terminate fuzzing" << std::endl;
  my_logger_g.logger->info("terminate fuzzing");
  SHM_Layer_Communication::is_active = 0;
}

/**
   TODO: here we get the 'terminate called w/o active exception'
 */
void Base_Coordinator::fuzzing_loop()
{

    // Dissector warm-up. For some reason, wdissector does not dissect the first packet correctly.
    auto p = helpers::get_sample_packet();
    if (!p.quick_dissect()) {
        my_logger_g.logger->error("Dissector failed to dissect a sample packet!");
        return;
    }

    std::unique_ptr<std::thread> thread_mle;
    std::unique_ptr<std::thread> thread_coap;

    bool mle_thread_is_running = false;
    bool coap_thread_is_running = false;

    bool fuzz_MLE = fuzz_config_g.FUZZ_MLE;
    bool fuzz_COAP = fuzz_config_g.FUZZ_COAP;

    if (fuzz_MLE) {
        mle_thread_is_running = true;
        thread_mle = std::make_unique<std::thread>(&Base_Coordinator::layer_fuzzing_loop, this, SHM_MUTEX_MLE);
    }

    if (fuzz_COAP) {
        coap_thread_is_running = true;
        thread_coap = std::make_unique<std::thread>(&Base_Coordinator::layer_fuzzing_loop, this, SHM_MUTEX_COAP);
    }

    std::cout << "BLUG: CALLING RESET ON NODES" << std::endl;

    if (!protocol_stack->restart()) {
        std::cout << "Failed to start a protocol stack" << std::endl;
        my_logger_g.logger->error("Failed to start a protocol stack");
        Base_Coordinator::terminate_fuzzing();
        goto exit;
    }

    if (!dut->restart()) {
        std::cout << "Failed to start a DUT" << std::endl;
        my_logger_g.logger->error("Failed to start a DUT");
        Base_Coordinator::terminate_fuzzing();
        goto exit;
    }

    std::cout << "restarts successfull" << std::endl;
    my_logger_g.logger->info("restarts succesfull");

    thread_dut_communication_func();

exit:
    std::cout << "killing everything" << std::endl;
    my_logger_g.logger->error("killing everything");

    if (mle_thread_is_running) {
        thread_mle->join();
    }

    if (coap_thread_is_running) {
        thread_coap->join();
    }

    // TODO: FIX THIS!! BAD FOR PHYS. NEEDED FOR VIRT.?
    // Base_Fuzzer::mutated_fields.clear();
    // Base_Fuzzer::mutated_fields_num = 0;
}

double Base_Coordinator::coverage_formula_1(double beta) {
    if (Base_Fuzzer::mutated_fields_num == 0) return 0.0;
    double score = 0;

    double normalized_value = std::min(1.0, (double)local_iteration / 2000);
    double cov_formula = beta * normalized_value;

    if (iteration_result.was_new_coverage_found) score += cov_formula / Base_Fuzzer::mutated_fields_num;
    else score -= (1.0 / cov_formula) / Base_Fuzzer::mutated_fields_num;

    return score;
}

double Base_Coordinator::calculate_field_mutation_score() {
    if (fuzz_strategy_config_g.use_coverage_feedback) {
        return coverage_formula_1(fuzz_strategy_config_g.beta);
    }
    return 0.0;
}

void Base_Coordinator::update_probabilities(bool found_new_coverage) {
    if (Base_Fuzzer::mutated_fields.size() == 0) return;
    double score = calculate_field_mutation_score();
    my_logger_g.logger->debug("Update score: {}", score);
    std::vector<std::shared_ptr<Field>>::reverse_iterator it;
    for (it = Base_Fuzzer::mutated_fields.rbegin(); it != Base_Fuzzer::mutated_fields.rend(); it++) {
        if ((*it) != nullptr) {
            double prob_change = score / log2((*it)->max_value + 1);
            my_logger_g.logger->debug("Field {} (idx {}): change={}", (*it)->field_name, (*it)->index, prob_change);
            (*it)->set_mutation_probability((*it)->mutation_probability + prob_change);
        }
    }
}

void Base_Coordinator::update_coverage_information() {
    if (statistics.is_open()) {
        statistics << std::endl << helpers::get_current_time_stamp();
    }
    iteration_result.was_new_coverage_found = false;
    uint8_t idx = 0;
    for (auto& cov_tracker_ptr : coverage_trackers) {
        uint32_t covered_edges_num = cov_tracker_ptr->get_edge_coverage(); // throws an exception if unsuccessful
        if (statistics.is_open()) {
            statistics << "," << covered_edges_num << ",";
        }
        Base_Fuzzer::current_seed->edges_covered = covered_edges_num;
        //bool new_coverage_found = false;
        uint64_t old_filled_buckets = cov_tracker_ptr->get_number_of_filled_buckets();
        if (cov_tracker_ptr->update_coverage_map()) {
            my_logger_g.logger->info("{}: NEW COVERAGE FOUND!", cov_tracker_ptr->get_name());
        } else {
            my_logger_g.logger->info("{}: NEW COVERAGE NOT FOUND!", cov_tracker_ptr->get_name());
        }
        
        uint64_t tec = cov_tracker_ptr->get_current_total_coverage();
        uint64_t new_filled_buckets = cov_tracker_ptr->get_number_of_filled_buckets();
        if (idx == 0) {
            iteration_result.new_coverage_found = new_filled_buckets - old_filled_buckets;
            iteration_result.was_new_coverage_found = (iteration_result.new_coverage_found != 0);
        }

        if (statistics.is_open()) {
            statistics << tec << "," << new_filled_buckets;// << "," << fuzzer->get_number_of_saved_patches();
        }
        cov_tracker_ptr->reset_coverage_map();
        idx++;
    }
}

bool Base_Coordinator::prepare_new_fuzzing_sprint(const Fuzz_Strategy_Config& fuzz_strategy_config)
{
    fuzz_strategy_config_g = fuzz_strategy_config;
    local_iteration = 0;
    fuzzers.clear();
    for (const FuzzingStrategy fuzz_strategy : fuzz_strategy_config_g.fuzzing_strategies) {
        fuzzers.push_back(Fuzzer_Factory::get_fuzzer_by_fuzzing_strategy(fuzz_strategy));
    }
    for (size_t i = 0; i < fuzzers.size(); i++) {
        if (fuzz_strategy_config_g.use_existing_seeds) {
            if (fuzz_strategy_config_g.seed_paths.size() > i) fuzzers.at(i)->set_seed_path(fuzz_strategy_config_g.seed_paths.at(i));
            else my_logger_g.logger->warn("Could not use an existing seed. Please check the seeds_paths variable in the config file.");
        }
        if (!fuzzers.at(i)->init()) {
            printf(RED "Failed to initialize fuzzer\n" CRESET);
            return false;
        }
    }

    if (statistics_file_name.empty()) {
        if (fuzz_strategy_config_g.coverage_log_path != "") {
            if (!helpers::create_directories_if_not_exist(fuzz_strategy_config_g.coverage_log_path)) {
                throw std::runtime_error("Could not create coverage log directory");
            }
            statistics_file_name = fuzz_strategy_config_g.coverage_log_path + "/coverage_statistics_" + helpers::get_current_time_stamp();
        } else {
            statistics_file_name = "/tmp/coverage_statistics_" + helpers::get_current_time_stamp();
        }

        if (fuzz_strategy_config_g.use_existing_patches) statistics_file_name += "_PREPATCHED";
        statistics.open(statistics_file_name);
    }

    save_all_seeds_to.open(save_all_seeds_to_filename, std::ios::trunc);

    return true;
}

static std::string get_banner() {
    return R"(_________          _______  _______  _______  ______       _______           _______  _______  _______  _______ 
\__   __/|\     /|(  ____ )(  ____ \(  ___  )(  __  \     (  ____ \|\     /|/ ___   )/ ___   )(  ____ \(  ____ )
   ) (   | )   ( || (    )|| (    \/| (   ) || (  \  )    | (    \/| )   ( |\/   )  |\/   )  || (    \/| (    )|
   | |   | (___) || (____)|| (__    | (___) || |   ) |    | (__    | |   | |    /   )    /   )| (__    | (____)|
   | |   |  ___  ||     __)|  __)   |  ___  || |   | |    |  __)   | |   | |   /   /    /   / |  __)   |     __)
   | |   | (   ) || (\ (   | (      | (   ) || |   ) |    | (      | |   | |  /   /    /   /  | (      | (\ (   
   | |   | )   ( || ) \ \__| (____/\| )   ( || (__/  )    | )      | (___) | /   (_/\ /   (_/\| (____/\| ) \ \__
   )_(   |/     \||/   \__/(_______/|/     \|(______/     |/       (_______)(_______/(_______/(_______/|/   \__/
                                                                                                                )";
}

void Base_Coordinator::print_statistics() {
    std::cout << BLUE << get_banner() << CRESET << std::endl;
    std::string total_iterations = (fuzz_strategy_config_g.total_iterations > 0) ? std::to_string(fuzz_strategy_config_g.total_iterations) : "∞";
    std::cout << std::setw(40) << std::left << BLUE "\tStatistics:" CRESET << std::endl;
    std::cout << std::setw(40) << std::left << "\tIteration: " + std::to_string(local_iteration) + "/" + total_iterations + " (" + std::to_string(global_iteration) + " in total)"; 
    // std::cout << std::setw(30) << std::left << "Successful attaches: " + std::to_string(statistics_g.attach_successful_counter)  << std::endl;
    std::cout << std::endl;
    
    if (fuzz_strategy_config_g.use_coverage_logging) std::cout << std::setw(40) << std::left << "\tTotal edges explored: " + std::to_string(coverage_trackers.at(0)->get_current_total_coverage());
    // std::cout << std::setw(30) << std::left << "Failed attaches: " + std::to_string(statistics_g.attach_failed_counter)  << std::endl;
    std::cout << std::endl;
    
    if (fuzz_strategy_config_g.use_coverage_logging) std::cout << std::setw(40) << std::left << "\tTotal buckets explored: " + std::to_string(coverage_trackers.at(0)->get_number_of_filled_buckets());
    // std::cout << std::setw(30) << std::left << "Hangs: " + std::to_string(statistics_g.hang_counter)  << std::endl;
    std::cout << std::endl;

    std::cout << std::setw(40) << std::left << "\tDUT --> Router: " + std::to_string(statistics_g.dut_become_router_counter);
    std::cout << std::endl;

    std::cout << std::setw(40) << std::left << RED "\tCrashes (PS/DUT): " + std::to_string(statistics_g.protocol_stack_crash_counter) + "/"
        + std::to_string(statistics_g.dut_crash_counter);
    std::cout << std::setw(30) << std::left << RED "Failed fuzz iterations: " + std::to_string(statistics_g.failed_fuzz_iterations_counter) + std::string(CRESET) << std::endl;
    std::cout << std::setw(30) << std::left
              << RED "\thangs: " + std::to_string(statistics_g.long_silence) +
                     std::string(CRESET)
              << std::endl;
    std::cout << std::setw(30) << std::left
              << RED "\tempty iterations: " + std::to_string(statistics_g.empty_iterations) +
                     std::string(CRESET)
              << std::endl;

    std::cout << std::setw(40) << std::left << BLUE "\tMutation statistics:" CRESET << std::endl;

    std::cout << std::setw(40) << std::left << "\tRandom mutations: " + std::to_string(statistics_g.rand_mutator_counter);
    std::cout << std::setw(30) << std::left << "Fuzzing strategies: " + get_all_fuzzing_strategy_names(fuzz_strategy_config_g.fuzzing_strategies)  << std::endl;

    std::cout << std::setw(40) << std::left << "\tMin mutations: " + std::to_string(statistics_g.min_mutator_counter);
    std::cout << std::endl;

    std::cout << std::setw(40) << std::left << "\tMax mutations: " + std::to_string(statistics_g.max_mutator_counter);
    std::cout << std::endl;

    std::cout << std::setw(40) << std::left << "\tAdd mutations: " + std::to_string(statistics_g.add_mutator_counter);
    std::cout << std::endl;


    std::cout << std::setw(40) << std::left << "\tSub mutations: " + std::to_string(statistics_g.sub_mutator_counter);
    std::cout << std::endl;

    std::cout << std::setw(40) << std::left << "\tInjections: " + std::to_string(statistics_g.injection_counter);
    std::cout << std::endl;

    std::cout << std::setw(40) << std::left << "\tLen mutations: " + std::to_string(statistics_g.len_mutation_counter);
    std::cout << std::endl;

    std::cout << std::setw(40) << std::left
              << "\tCID resps found: " +
                     std::to_string(statistics_g.nb_of_cid_responses);
    std::cout << std::endl;

    std::cout << std::setw(40) << std::left
              << "\tDUT reboots found: " +
                     std::to_string(statistics_g.dut_reboot_counter);
    std::cout << std::endl;

    std::cout << std::setw(40) << std::left
              << RED "\tepoch stats: iteration " + std::to_string(statistics_g.it_in_epochs + 1) + "/" + std::to_string(fuzz_strategy_config_g.epoch_size) + " in epoch " + std::to_string(statistics_g.epochs + 1)
                     + std::string(CRESET);
    std::cout << std::endl;

    std::cout << std::setw(40) << std::left
              << RED "\tWeird epochs: " +
                     std::to_string(statistics_g.refinement_runs) + std::string(CRESET);
    std::cout << std::endl;


    std::cout << std::setw(40) << std::left << "\tAvg iteration time: " + std::to_string(statistics_g.avg_iteration_time_s);
    std::cout << std::endl;
}
