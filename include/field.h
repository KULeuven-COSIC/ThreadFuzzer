#pragma once

#include "parsed_field.h"

#include <iostream>
#include <string>

#include <nlohmann/json.hpp>

#include "wdissector.h"

enum class FIELD_TYPE {
    FIELD = WD_TYPE_FIELD,
    GROUP = WD_TYPE_GROUP,
    LAYER = WD_TYPE_LAYER,
    NONE
};

/**
 * Class representing a field in the packet.
 */
class Field {
public:

    //! Default constructor
    Field() = default;
    /**
     * Constructor
     * @param field_type The type of the fiemax_valueld
     * @param field_name The name of the field
     * @param parsed_f Parsed_field instance
     */
    Field(FIELD_TYPE field_type, std::string field_name, parsed_field parsed_f);

    //! The type of the field
    FIELD_TYPE field_type;
    //! The name of the field
    std::string field_name;
    //! The index of the field in case there are multiple fields with the same name
    uint32_t index = 0;
    //! Parsed_field instance used to locate the field in the packet
    parsed_field parsed_f = parsed_field();
    //! Maximum value that the field can take
    uint64_t max_value = 0ULL;

public:
    //! Current probability of this field to be mutated
    double mutation_probability = 0.0;
    //! Coverage status of the field after the last iteration when it was fuzzed. False if new coverage was not found after mutating this field, True - otherwise
    bool coverage_status = false;

public:

    inline uint16_t get_offset() {
        return parsed_f.offset;
    }

    inline uint16_t get_offset_end() {
        return parsed_f.offset + parsed_f.length;
    }

    /**
     * Sets the mutation probability to the field
     * @param prob Mutation probability
     */
    void set_mutation_probability(double prob);

    inline bool operator==(const Field& f) const {
        return field_name == f.field_name && index == f.index;
    }

    //! Visualize the class instance
    std::ostream& dump(std::ostream& os) const;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Field, field_name, index, parsed_f, mutation_probability, coverage_status)

std::ostream& operator<<(std::ostream& os, const Field& f);
std::ostream& operator<<(std::ostream& os, const std::shared_ptr<Field>& f);