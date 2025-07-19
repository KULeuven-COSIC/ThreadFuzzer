#include "field.h"

#include "parsed_field.h"

#include <iomanip>
#include <iostream>
#include <stdexcept>

#include "Configs/Fuzzing_Strategies/fuzz_strategy_config.h"

#include "my_logger.h"

extern Fuzz_Strategy_Config fuzz_strategy_config_g;
extern My_Logger my_logger_g;

Field::Field(FIELD_TYPE field_type, std::string field_name, parsed_field parsed_f) : field_type(field_type), field_name(field_name), parsed_f(parsed_f) {
    uint8_t field_bit_len = get_field_size_bits(&parsed_f);
    // TODO: What if the field is too long?
    if (field_bit_len == 0 || field_bit_len >= 64) {
        my_logger_g.logger->debug("Field {} has bit-length {}", field_name, field_bit_len);
        //throw std::runtime_error("Unsupported field bit length");
    } else {
        max_value = (1U << field_bit_len);
    }
    my_logger_g.logger->debug("Init field {}: len={}, align={}, offset={}, mask={:x}, mask_offset={}, max_value={}", field_name, parsed_f.length, parsed_f.mask_align, parsed_f.offset, parsed_f.mask, parsed_f.mask_offset, max_value);
    
}

void Field::set_mutation_probability(double prob) {
    if (prob > fuzz_strategy_config_g.field_max_mut_prob) mutation_probability = fuzz_strategy_config_g.field_max_mut_prob;
    else if (prob < fuzz_strategy_config_g.field_min_mut_prob) mutation_probability = fuzz_strategy_config_g.field_min_mut_prob;
    else mutation_probability = prob;
}

std::ostream& Field::dump(std::ostream& os) const {
    os << "===FIELD===" << std::endl;
    os << "Name: " << field_name << std::endl;
    os << "Index: " << index << std::endl; ;
    os << "Mutation probability: " << std::fixed << std::setprecision(2) << mutation_probability << std::endl;
    os << parsed_f << std::endl;
    return os;
}

std::ostream& operator<<(std::ostream& os, const Field& f) {
    return f.dump(os);
}

std::ostream& operator<<(std::ostream& os, const std::shared_ptr<Field>& f) {
    return f->dump(os);
}