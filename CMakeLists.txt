cmake_minimum_required(VERSION 3.7)

project(ThreadFuzzer)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -fsanitize=address -pedantic-errors -Wall -Werror")

set(THIRD_PARTY_PATH "third-party")
set(COMMON_PATH "common")

include_directories(
	include
    ${THIRD_PARTY_PATH}/wdissector/
    ${THIRD_PARTY_PATH}/wdissector/libs/wireshark
    ${COMMON_PATH}
    ${COMMON_PATH}/shm
    ${COMMON_PATH}/ZMQ/ZMQ_Client/include
    /usr/include/glib-2.0
	/usr/lib/x86_64-linux-gnu/glib-2.0/include
)

link_directories(
    ${THIRD_PARTY_PATH}/wdissector/build
    ${THIRD_PARTY_PATH}/wdissector/libs/wireshark/build/run
    ${COMMON_PATH}/shm/build
    ${COMMON_PATH}/ZMQ/ZMQ_Client/build
)

add_executable(ThreadFuzzer
    src/coverage_requester.cpp
    src/coverage_tracker.cpp
    src/dissector.cpp
    src/field.cpp
    src/field_tree.cpp
    src/helpers.cpp
    src/main.cpp
    src/mutated_field_num_tracker.cpp
    src/mutation.cpp
    src/mutator.cpp
    src/my_logger.cpp
    src/packet.cpp
    src/parsed_field.cpp
    src/patch.cpp
    src/seed.cpp
    src/statistics.cpp
    src/Configs/Fuzzing_Settings/main_config.cpp
    src/Configs/Fuzzing_Settings/technical_config.cpp
    src/Configs/Fuzzing_Settings/timers_config.cpp
    src/Configs/Fuzzing_Strategies/fuzz_strategy_config.cpp
    src/Communication/shm_layer_communication.cpp
    src/Communication/shm_layer_communication_factory.cpp
    src/Communication/shm_MLE_communication.cpp
    src/Communication/shm_COAP_communication.cpp
    src/Communication/ot_pipe_helper.cpp
    src/Coordinators/base_coordinator.cpp
    src/Coordinators/dummy_coordinator.cpp
    src/Coordinators/timeout_based_coordinator.cpp
    src/Coordinators/phys_timeout_based_coordinator.cpp
    src/Coordinators/coordinator_factory.cpp
    src/Fuzzers/base_fuzzer.cpp
    src/Fuzzers/len_fuzzer.cpp
    src/Fuzzers/skip_fuzzer.cpp
    src/Fuzzers/dummy_fuzzer.cpp
    src/Fuzzers/fuzzer_factory.cpp
    src/Fuzzers/random_fuzzer.cpp
    src/Fuzzers/tlv_duplicator.cpp
    src/Fuzzers/tlv_inserter.cpp
    src/Fuzzers/tlv_mixer.cpp
    src/Fuzzers/reboot_cnt_fuzzer.cpp
    src/DUT/alexa.cpp
    src/DUT/nanoleaf.cpp
    src/DUT/eve_sensor.cpp
    src/DUT/DUT_base.cpp
    src/DUT/DUT_factory.cpp
    src/DUT/DUT_names.cpp
    src/DUT/dummy_DUT.cpp
    src/Protocol_Stack/dummy_stack.cpp
    src/Protocol_Stack/openthread_stack.cpp
    src/Protocol_Stack/protocol_stack_factory.cpp
    src/Protocol_Stack/OT_Instances/OT_dummy.cpp
    src/Protocol_Stack/OT_Instances/OT_TD.cpp
    src/Protocol_Stack/OT_Instances/OT_MTD.cpp
    src/Protocol_Stack/OT_Instances/OT_FTD.cpp
    src/Protocol_Stack/OT_Instances/OT_BR.cpp
    src/Protocol_Stack/Packet_Generator/OT_packet_generator_factory.cpp
    src/Protocol_Stack/RCP/RCP_factory.cpp
    src/Protocol_Stack/RCP/RCP_dummy.cpp
    src/Protocol_Stack/RCP/RCP_Sim.cpp
)

target_link_libraries(ThreadFuzzer
    shared_memory
    wdissector
    fmt
    zmq_client_lib
    zmq
)
