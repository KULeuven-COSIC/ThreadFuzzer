#pragma once

#include "seed.h"

#include <iostream>
#include <string>
#include <unordered_set>
#include <functional>

#include <nlohmann/json.hpp>

struct State {

    State(const State& s) : name(s.name), packet_size(s.packet_size), direction(s.direction), seed_to_get_to_state(s.seed_to_get_to_state), visit_counter(s.visit_counter) {}
    State() {}
    State(const std::string& name) : name(name), packet_size(0), direction(2) {} // 2 - is non-existent direction
    State(const std::string& name, size_t packet_size, size_t direction) : name(name), packet_size(packet_size), direction(direction) {}
    State(const std::string& name, size_t packet_size, size_t direction, const std::shared_ptr<Seed>& seed) : name(name), packet_size(packet_size), direction(direction), seed_to_get_to_state(seed) {}

    State& operator=(const State& s) {
        this->name = s.name;
        this->packet_size = s.packet_size;
        this->direction = s.direction;
        this->visit_counter = s.visit_counter;
        this->seed_to_get_to_state = s.seed_to_get_to_state;
        return *this;
    }

    std::string name;
    size_t packet_size;
    size_t direction; // UPLINK (0) or DOWNLINK (1) or 2 = test
    mutable std::shared_ptr<Seed> seed_to_get_to_state;
    mutable size_t visit_counter = 1; // how many times this state was visited during fuzzing

};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(State, name, packet_size, direction)

std::ostream& operator<<(std::ostream& os, const State& s);
bool operator==(const State& t1, const State& t2);

class StateHasher {
    public:
        size_t operator() (const State& s) const {
            return std::hash<std::string>()(s.name) ^ std::hash<size_t>()(s.packet_size) ^ std::hash<size_t>()(s.direction);
        }
};

struct Transition {
    State from;
    State to;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Transition, from, to)

bool operator==(const Transition& t1, const Transition& t2);
std::ostream& operator<<(std::ostream& os, const Transition& t);

class TransitionHasher {
    public:
        size_t operator() (const Transition& t) const {
            return StateHasher{}(t.from) ^ (StateHasher{}(t.to) << 1);
        }
};

class FSM {
public:

    FSM();

    void set_init_state(const State& state);
    // void set_end_state(const std::string& state);
    bool add_state(const State& state);
    void reset();

    std::unordered_set<State, StateHasher> get_all_states();
    std::vector<State const*> get_main_states(); //Return all states wo init_states, end_states, unknown_state
    std::vector<State const*> get_main_uplink_states(); //Return all states wo init_states, end_states, unknown_state
    std::vector<State const*> get_main_downlink_states(); //Return all states wo init_states, end_states, unknown_state

    inline size_t get_total_state_visits() const { return total_state_visits; }

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(FSM, init_state, end_states, states, transitions)

private:

    bool add_transition(const Transition& transition);

    State init_state;
    std::unordered_set<State, StateHasher> end_states;
    std::unordered_set<State, StateHasher> states;
    std::vector<State const*> ordered_main_states;
    std::vector<State const*> ordered_main_uplink_states;
    std::vector<State const*> ordered_main_downlink_states;
    std::unordered_set<Transition, TransitionHasher> transitions;

    size_t total_state_visits = 0;

    State current_state;
    Transition current_transition;
};