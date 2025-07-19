#include "FSM.h"

#include "my_logger.h"

extern My_Logger my_logger;

bool operator==(const State& s1, const State& s2) {
    return s1.name == s2.name && s1.direction == s2.direction;
}

bool operator==(const Transition& t1, const Transition& t2) {
    return t1.from == t2.from && t1.to == t2.to;
}

std::ostream& operator<<(std::ostream& os, const State& s) {
    os << "State: " << s.name;
    if (s.name == "") os << "<EMPTY>";
    os << " [Direction: " << s.direction << "]"; 
    if (s.seed_to_get_to_state) os << " (Seed ID: " << s.seed_to_get_to_state->get_id() << ")";
    return os;
}

std::ostream& operator<<(std::ostream& os, const Transition& t) {
    os << "From: " << t.from << "\n";
    os << "To: " << t.to << "\n";
    return os;
}

FSM::FSM() {
    init_state = State("INIT_STATE");

    states.insert(init_state);

    end_states.insert(State("ATTACH SUCCESSFUL"));
    end_states.insert(State("ATTACH FAILED"));
    end_states.insert(State("HANG"));

    for (const State& end_state : end_states) {
        states.insert(end_state);
    }

    current_transition.from = init_state;
}

void FSM::reset() {
    current_transition.from = init_state;
}

void FSM::set_init_state(const State& state) {
        init_state = state;
}

bool FSM::add_state(const State& state) {

    total_state_visits++;

    //Remember the state
    std::unordered_set<State, StateHasher>::iterator it;
    if (it = states.find(state); it == states.end()) {
        it = states.insert(state).first;
        ordered_main_states.push_back(&(*it));
        if (state.direction == DIRECTION_UPLINK) ordered_main_uplink_states.push_back(&(*it));
        else if (state.direction == DIRECTION_DOWNLINK) ordered_main_downlink_states.push_back(&(*it));
    } else {
        it->visit_counter++;
    }

    //Complete previous transition
    current_transition.to = *it;
    add_transition(current_transition);

    //Start new transition
    current_transition.from = *it;

    return true;
}

bool FSM::add_transition(const Transition& transition) {

    transitions.insert(transition);
    my_logger.logger->debug("Adding transition to the FSM: {}", transition);

    return true;
}

std::unordered_set<State, StateHasher> FSM::get_all_states() {
    return states;
}

std::vector<State const*> FSM::get_main_states() { //Return all states wo init_states, end_states, no_response_state
    return ordered_main_states;
}

std::vector<State const*> FSM::get_main_uplink_states() { //Return all states wo init_states, end_states, no_response_state
    return ordered_main_uplink_states;
}

std::vector<State const*> FSM::get_main_downlink_states() { //Return all states wo init_states, end_states, no_response_state
    return ordered_main_downlink_states;
}