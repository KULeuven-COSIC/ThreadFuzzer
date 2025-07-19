#pragma once

#include <atomic>
#include <cstddef>
#include <memory>
#include <vector>

#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>

/* Helper definitions */
#define SHM_MSG_MAX_SIZE 4090
#define SHM_CLIENT 0
#define SHM_SERVER 1
#define SHM_NAME "ThreadFuzz_SHM"

enum class PACKET_SRC {
    SRC_DUT,
    SRC_PROTOCOL_STACK
};

typedef enum
{
    SHM_MUTEX_MLE,
    SHM_MUTEX_COAP,
    SHM_MUTEX_MAX
} EnumMutex;

size_t get_random_bytes(uint8_t* arr, size_t max_size);
void print_bytes(const uint8_t* arr, size_t size);

class SHM {

public:
    SHM(bool is_server, const std::string name, size_t buffer_size_per_layer, size_t sync_var_size_per_layer = sizeof(std::atomic_bool), size_t layers_num = EnumMutex::SHM_MUTEX_MAX);
    ~SHM();

    void write_bytes(size_t layer_num, const uint8_t*, const uint32_t);
    bool read_bytes(size_t layer_num, uint8_t*, uint32_t&);

    static volatile int keep_running;

private:

    bool is_server;
    bool binary_wait_cond;
    const std::string name;

    size_t total_size_per_layer;
    size_t buffer_size_per_layer;
    size_t sync_var_size_per_layer;

    size_t layers_num;

    boost::interprocess::shared_memory_object shm_obj;
    std::vector< std::pair<boost::interprocess::mapped_region, boost::interprocess::mapped_region> > mapped_regions;

};

