#include "Fuzzers/tlv_inserter.h"

#include "helpers.h"
#include "Configs/Fuzzing_Strategies/fuzz_strategy_config.h"
#include "my_logger.h"

#include "spdlog/fmt/bin_to_hex.h"

#include <algorithm>
#include <vector>

extern Fuzz_Strategy_Config fuzz_strategy_config_g;
extern My_Logger my_logger_g;

bool TLV_Inserter::init() {
	return Base_Fuzzer::init();
}

bool TLV_Inserter::fuzz(Packet& packet) {

    if (apply_predefined_patches(packet)) return true;

    {
        /* Save some TLVs from the current packet. */
        const std::vector<std::shared_ptr<Field_Node>>& group_field_nodes = packet.get_group_field_nodes();
        const std::string keyword_end = ".tlv";

        std::vector<int> tlv_indices;
        for (int i = 0, i_max = group_field_nodes.size(); i < i_max; ++i) {
            const std::shared_ptr<Field_Node>& group_field_node = group_field_nodes.at(i); 
            if (group_field_node->field->field_name.ends_with(keyword_end)) {
                tlv_indices.push_back(i);
            }
        }

        if (tlv_indices.size() == 0) {
            my_logger_g.logger->warn("No group TLVs found. Cannot apply TLV Inserter");
            return false;
        }
        my_logger_g.logger->debug("Found {} group TLVs", tlv_indices.size());

        std::vector<uint8_t> packet_data = packet.get_data();

        /* Save some TLVs */
        helpers::random_shuffle(tlv_indices);
        int num_of_tlvs = helpers::EXP0(tlv_indices.size());

        for (int i = 0, i_max = num_of_tlvs; i < i_max; ++i) {
            const std::shared_ptr<Field_Node>& current_tlv = group_field_nodes.at(tlv_indices.at(i));
            int tlv_offset = current_tlv->field->get_offset();
            int tlv_length = current_tlv->field->parsed_f.length;
            std::vector<uint8_t> tlv(packet_data.begin() + tlv_offset, packet_data.begin() + tlv_offset + tlv_length);
            if (auto it = TLV_buffer.find(current_tlv->field->field_name); it != TLV_buffer.end()) {
                it->second.push_back(std::move(tlv));
            } else {
                TLV_buffer[current_tlv->field->field_name] = { std::move(tlv) };
            }
        }

        if (TLV_buffer.empty()) {
            my_logger_g.logger->debug("TLV buffer is empty! Cannot insert the TLV.");
            return false;
        }
    }

    const int iter_num = helpers::EXP0(10);
    my_logger_g.logger->debug("Inserting TLVs {} times", iter_num);
    for (int i = 0, i_max = iter_num; i < i_max; ++i) {

        std::vector<uint8_t> packet_data = packet.get_data();
        const std::vector<std::shared_ptr<Field_Node>>& group_field_nodes = packet.get_group_field_nodes();

        /* Get random TLV_type */
        auto it = std::next(TLV_buffer.begin(), helpers::UR0(TLV_buffer.size()));
        std::string TLV_to_insert_name = it->first;
        std::vector<uint8_t> TLV_to_insert = it->second.at(helpers::UR0(it->second.size()));
        // std::vector<uint8_t> TLV_to_insert = {
        //     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        //     0x00, 0x03, 0x00, 0x00, 0x00
        // };
        // std::vector<uint8_t> TLV_to_insert(32, 0x00);

        std::vector<int> indicies_with_same_TLV_name;
        indicies_with_same_TLV_name.reserve(group_field_nodes.size());

        for (int i = 0, i_max = group_field_nodes.size(); i < i_max; ++i) {
            if (group_field_nodes.at(i)->field->field_name == TLV_to_insert_name) {
                indicies_with_same_TLV_name.push_back(i);
            }
        }

        if (indicies_with_same_TLV_name.empty()) {
            my_logger_g.logger->debug("No TLVs with the name {} found", TLV_to_insert_name);
            return false;
        }

        /* Get random node with the matching TLV name */
        std::shared_ptr<Field_Node> node = group_field_nodes.at(indicies_with_same_TLV_name.at(helpers::UR0(indicies_with_same_TLV_name.size())));

        if (!node) {
            my_logger_g.logger->error("NODE IS EMPTY! SHOULD NOT HAPPEN!");
            return false;
        }

        /* Insert the TLV after the chosen one. */
        int node_end = node->field->get_offset_end();
        my_logger_g.logger->info("Inserting TLV: {} on offset {}", spdlog::to_hex(TLV_to_insert), node_end);
        packet_data.insert(packet_data.begin() + node_end, TLV_to_insert.begin(), TLV_to_insert.end());

        double adjust_len_prob = helpers::URD();
        if (adjust_len_prob < fuzz_strategy_config_g.adjust_TLV_lengths_prob) {
            /* Update the length values of the necessary nodes. */
            std::weak_ptr<Field_Node> cur_node;
            if (helpers::URD() < 0.5) {
                /* Insert as SubTLV */
                cur_node = node;
                my_logger_g.logger->debug("Inserting as SubTLV");
            } else {
                /* Insert as a TLV */
                cur_node = node->parent;
                my_logger_g.logger->debug("Inserting as a neighboring TLV");
            }

            my_logger_g.logger->debug("Increasing the length of corresponding TLVs for {}", TLV_to_insert.size());
            //my_logger_g.logger->debug("Attaching TLV after {}", cur_node.lock());

            while (!cur_node.expired()) {
                if (!(cur_node.lock()->field)) break;
                // Update the Length value (assuming the TLV type of the group)
                if (cur_node.lock()->field->field_type == FIELD_TYPE::GROUP) {
                    //my_logger_g.logger->debug("Changing the length in field {}", cur_node.lock());
                    uint8_t cur_value = packet_data.at((cur_node.lock())->field->get_offset() + 1);
                    my_logger_g.logger->debug("Changing value {:X} to {:X}", cur_value, cur_value + TLV_to_insert.size());
                    packet_data.at((cur_node.lock())->field->get_offset() + 1) += TLV_to_insert.size(); // Might cause a byte overflow, but we don't care.
                }
                cur_node = (cur_node.lock())->parent;
            }
        }

        packet.update_packet_data(std::move(packet_data), true);
    }

    return true;
}