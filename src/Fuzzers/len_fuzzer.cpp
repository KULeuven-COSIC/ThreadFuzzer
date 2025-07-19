#include "Fuzzers/len_fuzzer.h"

#include "packet.h"

#include "my_logger.h"
#include "statistics.h"

extern My_Logger my_logger_g;
extern Statistics statistics_g;

bool LenFuzzer::init() {
	return Base_Fuzzer::init();
}

bool LenFuzzer::fuzz(Packet& packet) {

	if (apply_predefined_patches(packet)) return true;
    
    std::vector<uint8_t> new_packet_data = packet.get_data();

    // 1. Find the target LEN field
    std::shared_ptr<Field_Node> field_node_to_mutate;
    int size_shift;
    for (const std::shared_ptr<Field_Node>& field_node : packet.get_field_nodes()) {
        if (field_node->field->field_name.find(".len") != std::string::npos) {
            if (helpers::URD() < 0.1) {
                uint64_t cur_value = get_field_value(packet.get_data(), field_node->field->parsed_f);
                if (field_node->field->max_value == 0) {
                    my_logger_g.logger->warn("LEN field {} (idx: {}) has 0-max value", field_node->field->field_name, field_node->field->index);
                    continue;
                }
                uint64_t rand_value = helpers::UR0(field_node->field->max_value);
                set_field_value(new_packet_data, field_node->field->parsed_f, rand_value);
                field_node_to_mutate = field_node;
                size_shift = rand_value - cur_value;
                my_logger_g.logger->info("Mutated LEN field {} (idx: {}) from {} to {}", 
                    field_node->field->field_name, field_node->field->index, cur_value, rand_value);
                break;
            }
        }
    }

    // 2. Go to the corresponding DATA field and adjust the packet accordingly
    if (size_shift && field_node_to_mutate) {
        std::shared_ptr<Field_Node> parent = field_node_to_mutate->parent.lock();
        std::shared_ptr<Field_Node> last_child = parent->get_children().back();
        while (last_child->field->field_type != FIELD_TYPE::FIELD) {
            last_child = last_child->get_children().back();
        }
        parsed_field& data_parsed_field = last_child->field->parsed_f;
        if (static_cast<int>(data_parsed_field.length) + size_shift < 0) {
            my_logger_g.logger->warn("Failed to extend the data with size {} by length {} for field {}", data_parsed_field.length, size_shift, last_child->field->field_name);
        } else {
            /* Adjust the data in the packet */
            if (size_shift > 0) {
                /* Add extra bytes */
                std::vector<uint8_t> extra_bytes(size_shift, 0xFF);
                new_packet_data.insert(new_packet_data.begin() + data_parsed_field.offset, extra_bytes.begin(), extra_bytes.end());
            } else if (size_shift < 0) {
                /* Remove bytes */
                new_packet_data.erase(
                    new_packet_data.begin() + data_parsed_field.offset,
                    new_packet_data.begin() + data_parsed_field.offset + std::abs(size_shift)
                );
            }
            if (new_packet_data.size() != packet.get_data().size() + size_shift) {
                my_logger_g.logger->warn("Sizes do not line up");
                my_logger_g.logger->warn("Prev size: {}, New size: {}, Shift: {}", packet.get_data().size(), new_packet_data.size(), size_shift);
                return false;
            }
            packet.set_data(new_packet_data);
            statistics_g.len_mutation_counter++;
            packet.full_dissect();
        }
    }

    return true;
}