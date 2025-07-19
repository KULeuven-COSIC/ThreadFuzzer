#include "packet.h"

#include "dissector.h"
#include "helpers.h"
#include "my_logger.h"

#include "Configs/Fuzzing_Strategies/fuzz_strategy_config.h"
#include "Fuzzers/base_fuzzer.h"

#include "shm/shared_memory.h"

#include <string>
#include <unordered_map>

extern Fuzz_Strategy_Config fuzz_strategy_config_g;
extern My_Logger my_logger_g;

static std::shared_ptr<Field_Tree> field_tree_g;
static std::unordered_map<std::string, int> field_name_counter;
static std::unordered_map<std::string, int> group_field_name_counter;


std::vector<std::string> Packet::expected_prefixes;;

Packet::Packet() {};

Packet::Packet (uint8_t* data, uint16_t size, uint8_t layer, PACKET_SRC packet_src)  {
    data_.reserve(size);
    data_ = std::vector<uint8_t>(data, data + size);
    layer_ = layer;
    packet_src_ = packet_src;
}

Packet::Packet (const std::vector<uint8_t>& data, uint8_t layer, PACKET_SRC packet_src) 
    : data_(data), layer_(layer), packet_src_(packet_src) {}

Packet::Packet(const Packet& p) {
    data_ = p.data_;
    field_tree_ = p.field_tree_;
    layer_ = p.layer_;
    packet_summary_ = p.packet_summary_;
    packet_summary_short_ = p.packet_summary_short_;
    dissector_name_ = p.dissector_name_;
    packet_src_ = p.packet_src_;
}

Packet::Packet(Packet *p) {
    data_ = p->data_;
    field_tree_ = p->field_tree_;
    layer_ = p->layer_;
    packet_summary_ = p->packet_summary_;
    packet_summary_short_ = p->packet_summary_short_;
    dissector_name_ = p->dissector_name_;
    packet_src_ = p->packet_src_;
}

Packet& Packet::operator=(const Packet& p) {
    data_ = p.data_;
    field_tree_ = p.field_tree_;
    layer_ = p.layer_;
    packet_summary_ = p.packet_summary_;
    packet_summary_short_ = p.packet_summary_short_;
    dissector_name_ = p.dissector_name_;
    packet_src_ = p.packet_src_;
    return *this;
}

Packet& Packet::operator=(Packet* p) {
    data_ = p->data_;
    field_tree_ = p->field_tree_;
    layer_ = p->layer_;
    packet_summary_ = p->packet_summary_;
    packet_summary_short_ = p->packet_summary_short_;
    dissector_name_ = p->dissector_name_;
    packet_src_ = p->packet_src_;
    return *this;
}

bool Packet::operator==(const Packet &p) const
{
    return data_ == p.data_ && packet_summary_short_ == p.packet_summary_short_;
}

void Packet::set_initial_field_mutation_probabilities() {
    std::vector<std::shared_ptr<Field_Node>> field_nodes = field_tree_->get_field_nodes_mut();
    double mut_prob = 1.0 * fuzz_strategy_config_g.init_prob_mult_factor / field_nodes.size();
    my_logger_g.logger->debug("Setting probs to value {}", mut_prob);
    for (auto& field_node : field_nodes) {
        field_node->field->set_mutation_probability(mut_prob);
    }
}

void Packet::set_packet_summary(const std::string &packet_summary_)
{
    this->packet_summary_ = packet_summary_;
    packet_summary_short_ = helpers::shorten_dissector_summary(packet_summary_);
}

bool Packet::update_packet_data(const std::vector<uint8_t>& new_data, bool full_dissect) {
    data_ = new_data;
    if (full_dissect) {
        if (!this->full_dissect()) {
            my_logger_g.logger->warn("Full dissection failed");
        }
    } else {
        if (!this->quick_dissect()) {
            my_logger_g.logger->warn("Quick dissection failed");
        }
    }
    return true;
}

bool Packet::update_packet_data(std::vector<uint8_t>&& new_data, bool full_dissect)  {
    return update_packet_data(static_cast<const std::vector<uint8_t>&>(new_data), full_dissect);
}

bool Packet::quick_dissect() {
    if (!Dissector::set_dissector(dissector_name_)) {
        my_logger_g.logger->warn("Failed to set a dissector: {}", dissector_name_);
        return false;
    } 
    
    if (!Dissector::dissect_packet(data_)) {
        my_logger_g.logger->warn("Could not dissect the packet!");
        return false;
    }

    set_packet_summary(Dissector::get_last_packet_summary());

    return true;
}

bool Packet::full_dissect() {
    if (!quick_dissect()) return false;

    if (auto it = Base_Fuzzer::packet_field_tree_database.find(packet_summary_short_ + std::to_string(data_.size())); it != Base_Fuzzer::packet_field_tree_database.end()) {
        field_tree_ = it->second;
    } else {
        my_logger_g.logger->debug("Initializing packet: {} (len={})", packet_summary_short_, data_.size());
        init_fields();
        set_initial_field_mutation_probabilities();
        Base_Fuzzer::packet_field_tree_database[packet_summary_short_ + std::to_string(data_.size())] = field_tree_;
    }

    return true;
}

std::ostream& Packet::dump_fuzzed_packet(std::ostream& os, std::vector<parsed_field*>& fuzzed_fields) const {
    for (size_t i = 0; i < get_size(); i++) {
        os << CRESET << std::hex << std::uppercase;
        for (const parsed_field* parsed_field_ptr : fuzzed_fields) {
            if (i >= parsed_field_ptr->offset && i < parsed_field_ptr->offset + parsed_field_ptr->length) {
                os << BLUE;
            }
        }
        os << std::setfill('0') << std::setw(2) << data_.at(i);
    }
    os << CRESET << std::dec << std::nouppercase << std::endl;
    return os;
}

std::ostream& Packet::dump(std::ostream& os) const {
    os << std::hex << std::uppercase;
    for (size_t i = 0; i < get_size(); i++) {
        os << std::setfill('0') << std::setw(2) << static_cast<int>(data_.at(i)) << " ";
    }
    os << std::dec << std::nouppercase;
    return os;
}

std::ostream& operator<<(std::ostream& os, const Packet& packet) {
    return packet.dump(os);
}

uint8_t Packet::test_navigate_callback(proto_tree *subnode, uint8_t field_type, uint8_t *pkt_buf) {
    if (field_type == WD_TYPE_FIELD) {
        const std::string& short_field_name = subnode->finfo->hfinfo->abbrev;
        for (const auto& prefix : Packet::expected_prefixes) {
            if (short_field_name.starts_with(prefix)) {
                //Packet::field_names.insert(short_field_name);
                field_name_counter[short_field_name]++;
                parsed_field pf;
                if (!Packet::parse_field(subnode->finfo, &pf)) {
                    my_logger_g.logger->warn("Counldn't parse field {}", short_field_name);
                    break;
                }
                try {
                    Field f(FIELD_TYPE::FIELD, short_field_name, pf);
                    f.index = field_name_counter[short_field_name] - 1;
                    field_tree_g->add_field(std::make_shared<Field>(f));
                } catch(std::exception& ex) {
                    my_logger_g.logger->warn("Exception: {}", ex.what());
                    continue;
                }
                
                //f.value = packet_read_field_uint64(subnode->finfo) - (uint64_t)subnode->finfo->value_min;
                
                break;
            }
        }
        
    } else if (field_type == WD_TYPE_GROUP) {
        const std::string& short_field_name = subnode->finfo->hfinfo->abbrev;
        my_logger_g.logger->debug("Group field: {}", short_field_name);
        for (const auto& prefix : Packet::expected_prefixes) {
            if (short_field_name.starts_with(prefix)) {
                group_field_name_counter[short_field_name]++;
                parsed_field pf;
                if (!Packet::parse_field(subnode->finfo, &pf)) {
                    my_logger_g.logger->warn("Counldn't parse field {}", short_field_name);
                    break;
                }
                try {
                    Field f(FIELD_TYPE::GROUP, short_field_name, pf);
                    f.index = field_name_counter[short_field_name] - 1;
                    field_tree_g->add_field(std::make_shared<Field>(f));
                } catch(std::exception& ex) {
                    my_logger_g.logger->warn("Exception: {}", ex.what());
                    continue;
                }
                break;
            }
        }
    } else if (field_type == WD_TYPE_LAYER) {
        const std::string& short_field_name = subnode->finfo->hfinfo->abbrev;
        my_logger_g.logger->debug("Layer field: {}", short_field_name);
        parsed_field pf;
        if (!Packet::parse_field(subnode->finfo, &pf)) {
            my_logger_g.logger->warn("Counldn't parse field {}", short_field_name);
        } else {
            Field f(FIELD_TYPE::LAYER, short_field_name, pf);
            field_tree_g->add_field(std::make_shared<Field>(f));
        }
    }

    return 0;
}
    

void Packet::init_fields()
{
    Packet::expected_prefixes = helpers::get_field_prefixes_by_layer_idx(this->layer_);

    field_tree_g = std::make_shared<Field_Tree>();
    field_name_counter.clear();
    group_field_name_counter.clear();

    packet_navigate(5, 0, test_navigate_callback);
    my_logger_g.logger->debug("Inited {} fields", field_tree_g->get_field_nodes().size());
    my_logger_g.logger->debug("Inited {} group fields", field_tree_g->get_group_field_nodes().size());

    field_tree_ = std::move(field_tree_g);
}

bool Packet::parse_field(field_info *field, parsed_field *f)
{
    f->offset = packet_read_field_offset(field);
    if (f->offset > 38) {
        f->offset -= 38; // Account for the compression: TODO: change the way it is done
    } else {
        my_logger_g.logger->warn("Offset is {}", f->offset);
    }
    f->mask = packet_read_field_bitmask(field);
    f->length = packet_read_field_size(field);

	if (f->mask != 0) {
		uint16_t mask_offset = 0;
		uint64_t cur_mask = f->mask;
		cur_mask >>= 8 * (f->length - 1);
		assert(cur_mask != 0);
		for (size_t i = 0; i < 8; i++) {
			if ((cur_mask >> i) == 0) {
				mask_offset++;
			} else {
				mask_offset = 0;
			}
		}
		f->mask_offset = mask_offset;
	}

    f->mask_align = field->ds_tvb->bitshift_from_octet;
    return true;
}