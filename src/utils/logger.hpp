#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <string>
#include <sstream>

enum LogLevel {
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
    LOG_NONE
};

class Logger {
public:
    static void setLevel(LogLevel level);
    static void debug(const std::string& msg);
    static void info(const std::string& msg);
    static void warn(const std::string& msg);
    static void error(const std::string& msg);

private:
    static LogLevel currentLevel;
    static void log(LogLevel level, const std::string& prefix, const std::string& msg);
};

// 宏简化使用
#define LOG_DEBUG(msg) Logger::debug(msg)
#define LOG_INFO(msg)  Logger::info(msg)
#define LOG_WARN(msg)  Logger::warn(msg)
#define LOG_ERROR(msg) Logger::error(msg)

#endif