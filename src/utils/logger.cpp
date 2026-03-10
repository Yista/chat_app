#include "logger.hpp"
#include <iostream>
#include <chrono>
#include <iomanip>

LogLevel Logger::currentLevel = LOG_INFO;

void Logger::setLevel(LogLevel level) {
    currentLevel = level;
}

void Logger::log(LogLevel level, const std::string& prefix, const std::string& msg) {
    if (level < currentLevel) return;

    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm bt;
    localtime_r(&in_time_t, &bt); // 线程安全，Linux下可用

    std::ostringstream oss;
    oss << "[" << std::put_time(&bt, "%Y-%m-%d %H:%M:%S") << "] "
        << prefix << " " << msg;
    std::cout << oss.str() << std::endl;
}

void Logger::debug(const std::string& msg) {
    log(LOG_DEBUG, "[DEBUG]", msg);
}

void Logger::info(const std::string& msg) {
    log(LOG_INFO, "[INFO]", msg);
}

void Logger::warn(const std::string& msg) {
    log(LOG_WARN, "[WARN]", msg);
}

void Logger::error(const std::string& msg) {
    log(LOG_ERROR, "[ERROR]", msg);
}