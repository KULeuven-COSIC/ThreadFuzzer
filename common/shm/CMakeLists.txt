cmake_minimum_required(VERSION 3.7)

project(SHM)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -fsanitize=address")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -fsanitize=address -Wall")

add_library(shared_memory STATIC shared_memory.cpp shm_helper.cpp fuzz_config.cpp)