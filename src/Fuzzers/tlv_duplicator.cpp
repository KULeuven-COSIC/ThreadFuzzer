#include "Fuzzers/tlv_duplicator.h"

#include "Configs/Fuzzing_Strategies/fuzz_strategy_config.h"
#include "my_logger.h"

extern Fuzz_Strategy_Config fuzz_strategy_config_g;
extern My_Logger my_logger_g;

bool TLV_Duplicator::init() {
	return Base_Fuzzer::init();
}

bool TLV_Duplicator::fuzz(Packet& packet) {

    if (apply_predefined_patches(packet)) return true;

    /* NOTE: We assume that all the TLVs go right after each other. If it is not the case, this function will FAIL. */

    const std::vector<std::shared_ptr<Field_Node>>& group_field_nodes = packet.get_group_field_nodes();
    const std::string keyword = "mle.tlv";

    std::vector<int> tlv_indices;

    for (int i = 0, i_max = group_field_nodes.size(); i < i_max; ++i) {
        const std::shared_ptr<Field_Node>& group_field_node = group_field_nodes.at(i);
        //if (group_field.field_name.find(keyword) != std::string::npos) {
        if (group_field_node->field->field_name == keyword) {
            tlv_indices.push_back(i);
        }
    }

    if (tlv_indices.size() == 0) {
        my_logger_g.logger->warn("No TLVs found. Cannot apply TLV Mixer");
        return false;
    }
    my_logger_g.logger->debug("Found {} TLVs", tlv_indices.size());

    const std::shared_ptr<Field_Node>& last_tlv = group_field_nodes.at(tlv_indices.back());
    uint16_t tlvs_end_idx = last_tlv->field->parsed_f.offset + last_tlv->field->parsed_f.length;

    /* Build a new packet */
    // 1. Copy everything before the insertion point
    const std::vector<uint8_t>& old_packet_data = packet.get_data();
    std::vector<uint8_t> new_packet_data;
    new_packet_data.reserve(old_packet_data.size());
    new_packet_data.insert(
        new_packet_data.end(),
        old_packet_data.begin(),
        old_packet_data.begin() + tlvs_end_idx
    );

    /* Randomly shuffle the vector of TLV indices */
    helpers::random_shuffle(tlv_indices);
    int num_of_additional_tlvs = helpers::UR0(tlv_indices.size());

    // 2. Copy the TLVs in random order
    for (int i = 0, i_max = num_of_additional_tlvs; i < i_max; ++i) {
        const std::shared_ptr<Field_Node>& current_tlv = group_field_nodes.at(tlv_indices.at(i));
        new_packet_data.insert(
            new_packet_data.end(),
            old_packet_data.begin() + current_tlv->field->parsed_f.offset,
            old_packet_data.begin() + current_tlv->field->parsed_f.offset + current_tlv->field->parsed_f.length
        );
    }

    // 3. Copy everything after the last TLV
    new_packet_data.insert(
        new_packet_data.end(),
        old_packet_data.begin() + tlvs_end_idx,
        old_packet_data.end()
    );

    /* Insert the new data to the packet */
    // packet.set_data(std::move(new_packet_data));
    packet.update_packet_data(std::move(new_packet_data), true);

	return true;
}