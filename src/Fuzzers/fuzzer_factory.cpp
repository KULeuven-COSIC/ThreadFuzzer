#include "Fuzzers/fuzzer_factory.h"

#include "Configs/Fuzzing_Strategies/fuzz_strategy_config.h"

#include "Fuzzers/base_fuzzer.h"
#include "Fuzzers/dummy_fuzzer.h"
#include "Fuzzers/len_fuzzer.h"
#include "Fuzzers/random_fuzzer.h"
#include "Fuzzers/tlv_duplicator.h"
#include "Fuzzers/tlv_inserter.h"
#include "Fuzzers/tlv_mixer.h"
#include "Fuzzers/reboot_cnt_fuzzer.h"
#include "Fuzzers/skip_fuzzer.h"

#include <memory>
#include <stdexcept>

std::unique_ptr<Base_Fuzzer> Fuzzer_Factory::get_fuzzer_by_fuzzing_strategy(FuzzingStrategy fs) {
    switch (fs) {
        case FuzzingStrategy::NONE: return std::make_unique<DummyFuzzer>();
        case FuzzingStrategy::RANDOM_FUZZING: return std::make_unique<RandomFuzzer>();
        case FuzzingStrategy::TLV_MIXING: return std::make_unique<TLV_Mixer>();
        case FuzzingStrategy::TLV_DUPLICATING: return std::make_unique<TLV_Duplicator>();
        case FuzzingStrategy::TLV_INSERTING: return std::make_unique<TLV_Inserter>();
        case FuzzingStrategy::LEN_FUZZING: return std::make_unique<LenFuzzer>();
        case FuzzingStrategy::REBOOT_CNT_FUZZING: return std::make_unique<RebootCntFuzzer>();
        default: break;
    }
    throw std::runtime_error("Unknown fuzzing strategy");
}
