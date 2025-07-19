#pragma once

#include "field.h"
#include "field_tree.h"

#include "wdissector.h"
#include "shm/shared_memory.h"

#include <algorithm>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

class Packet {
public:
    Packet();
    Packet(uint8_t* data, uint16_t size, uint8_t layer, PACKET_SRC packet_src);
    Packet(const std::vector<uint8_t>& data, uint8_t layer, PACKET_SRC packet_src);
    Packet(const Packet& p);
    Packet(Packet *p);

    Packet& operator=(const Packet& p);
    Packet& operator=(Packet* p);

    bool operator==(const Packet& p) const;

    bool update_packet_data(std::vector<uint8_t>&& new_data, bool full_dissect = false);
    bool update_packet_data(const std::vector<uint8_t>& new_data, bool full_dissect = false);

public:
    inline uint32_t get_size() const {
        return data_.size();
    }

    inline uint8_t* get_raw_data() {
        return data_.data();
    }

    inline std::vector<uint8_t>& get_data_ref() {
        return data_;
    }

    inline const std::vector<uint8_t>& get_data() const {
        return data_;
    }

    inline const std::string& get_dissector_name() const {
        return dissector_name_;
    }

    inline uint8_t get_layer() const {
        return layer_;
    }

    inline const std::string& get_summary() const {
        return packet_summary_;
    }

    inline const std::string& get_summary_short() const {
        return packet_summary_short_;
    }

    inline const std::shared_ptr<Field_Tree> get_field_tree() const {
        return field_tree_;
    }

    inline const std::vector<std::shared_ptr<Field_Node>>& get_field_nodes() const {
        assert(field_tree_);
        return field_tree_->get_field_nodes();
    }

    inline const std::vector<std::shared_ptr<Field_Node>>& get_group_field_nodes() const {
        assert(field_tree_);
        return field_tree_->get_group_field_nodes();
    }

    inline int get_fields_offset() const {
        assert(field_tree_);
        return field_tree_->get_field_tree_offset();
    }

    inline int get_fields_end() const {
        assert(field_tree_);
        return field_tree_->get_field_tree_end();
    }

    inline const PACKET_SRC get_packet_src() const {
        return packet_src_;
    }

public:
    inline void set_dissector_name(const std::string& dissector_name) {
        dissector_name_ = dissector_name;
    }

    inline void set_data(std::vector<uint8_t>&& new_data) {
        data_ = new_data;
    }

    inline void set_data(const std::vector<uint8_t>& new_data) {
        data_ = new_data;
    }

public:
    friend struct PacketHash;
    friend void to_json(nlohmann::json& j, const Packet& packet);
    friend void from_json(const nlohmann::json& j, Packet& packet);

    void init_fields();
    std::ostream& dump(std::ostream& os) const;
    std::ostream& dump_fuzzed_packet(std::ostream&, std::vector<parsed_field*>& fuzzed_fields) const; // Print packet highlighting the fuzzed bytes
    void set_initial_field_mutation_probabilities();

    void set_packet_summary(const std::string& packet_summary);

    /* Quick dissect: gets information about the packet summary (name, protocol, etc.). Does not dissect any of the fields! */
    bool quick_dissect();
    /* Full dissect: performs quick_dissect and on top of that initializes all the fields in the packet (and assignes the mutation probabilities to them)*/
    bool full_dissect();

private:
    std::vector<uint8_t> data_;
    std::shared_ptr<Field_Tree> field_tree_;
    uint8_t layer_;
    std::string dissector_name_;
    
    std::string packet_summary_short_ = "";
    std::string packet_summary_ = "";

    PACKET_SRC packet_src_;

public:
    static uint8_t test_navigate_callback(proto_tree *subnode, uint8_t field_type, uint8_t *pkt_buf);
    static bool parse_field(field_info *field, parsed_field *f);
    static std::vector<std::string> expected_prefixes;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(Packet, data_, packet_summary_, packet_summary_short_)
};

std::ostream& operator<<(std::ostream& os, const Packet& packet);

struct PacketHash {
    size_t operator()(const Packet& packet) const {
        std::hash<uint8_t> hasher;
        size_t answer = 0;
        for (uint8_t i : packet.get_data()) {
            answer ^= hasher(i) + 0x9e3779b9 + (answer << 6) + (answer >> 2);
        }
        answer ^= std::hash<std::string>()(packet.get_summary_short());
        return answer;
    }
};
