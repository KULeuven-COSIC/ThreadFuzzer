#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/fmt/ostr.h>

class My_Logger {
public:
    bool init(const std::filesystem::path& log_dir_path);

    std::shared_ptr<spdlog::logger> logger;

    inline const std::filesystem::path& get_log_dir_path() const {
        return log_dir_path_;
    }

private:
    
    std::vector<spdlog::sink_ptr> sinks_;

    std::shared_ptr<spdlog::sinks::basic_file_sink_mt> file_debug_sink_;
    std::shared_ptr<spdlog::sinks::basic_file_sink_mt> file_info_sink_;

    std::filesystem::path file_debug_path_;
    std::filesystem::path file_info_path_;

    std::filesystem::path log_dir_path_;
};