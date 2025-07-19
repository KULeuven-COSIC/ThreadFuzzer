#include "Fuzzers/skip_fuzzer.h"

#include "Configs/Fuzzing_Strategies/fuzz_strategy_config.h"
#include "my_logger.h"

extern Fuzz_Strategy_Config fuzz_strategy_config_g;
extern My_Logger my_logger_g;

bool SkipFuzzer::prepare_new_iteration() {
    clean_packet_counter_map();
    return RandomFuzzer::prepare_new_iteration();
}

void SkipFuzzer::clean_packet_counter_map() {
    for (auto& [key, val] : packet_counter_map) {
        val = 0;
    }
}

bool SkipFuzzer::fuzz(Packet& packet) {

    const std::unordered_map<std::string, std::size_t>& skip_rules = fuzz_strategy_config_g.skip_rules;
    const std::string& packet_name = packet.get_summary();

    if (!packet_name.empty()) {
        if (auto it = skip_rules.find(packet_name); it != skip_rules.end()) {
            std::size_t skip_freq = it->second;

            std::size_t& packet_counter = packet_counter_map[packet_name];
            packet_counter++;

            if (packet_counter >= skip_freq) {
                my_logger_g.logger->info("Skipping packet in SkipFuzzer");
                packet_counter = 0;
                return true;
            } else {
                my_logger_g.logger->info("Packet NOT skipped in SkipFuzzer (current counter: {})", packet_counter);
            }
        }
    }

    return RandomFuzzer::fuzz(packet);
}
