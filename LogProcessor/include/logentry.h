#ifndef LOGENTRY_H
#define LOGENTRY_H

#define _CRT_SECURE_NO_WARNINGS

#include <string>
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>

enum class LogLevel {
    INFO,
    WARN,
    ERROR,
    UNKNOWN
};

inline std::string logLevelToString(LogLevel level) {
    switch (level) {
    case LogLevel::INFO: return "INFO";
    case LogLevel::WARN: return "WARN";
    case LogLevel::ERROR: return "ERROR";
    default: return "UNKNOWN";
    }
}

inline LogLevel stringToLogLevel(const std::string& level) {
    if (level == "INFO") return LogLevel::INFO;
    if (level == "WARN") return LogLevel::WARN;
    if (level == "ERROR") return LogLevel::ERROR;
    return LogLevel::UNKNOWN;
}

struct LogEntry {
    std::string level_str;
    LogLevel level = LogLevel::UNKNOWN;
    std::string message;
    std::string raw_timestamp;
    std::chrono::system_clock::time_point timestamp;

    LogEntry() : level(LogLevel::UNKNOWN) {}

    LogEntry(const std::string& lvl, const std::string& msg, const std::string& ts)
        : level_str(lvl), message(msg), raw_timestamp(ts) {
        level = stringToLogLevel(lvl);
        timestamp = parseTimestamp(ts);
    }

    static std::chrono::system_clock::time_point parseTimestamp(const std::string& ts) {
        std::tm tm = {};
        std::istringstream ss(ts);
        ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");

        if (!ss.fail()) {
            return std::chrono::system_clock::from_time_t(std::mktime(&tm));
        }

        return std::chrono::system_clock::time_point();
    }

    static std::string formatTimestamp(const std::chrono::system_clock::time_point& tp) {
        if (tp == std::chrono::system_clock::time_point()) {
            return "";
        }

        auto time_t = std::chrono::system_clock::to_time_t(tp);
        std::tm tm;
#ifdef _WIN32
        localtime_s(&tm, &time_t);
#else
        localtime_r(&time_t, &tm);
#endif
        std::ostringstream ss;
        ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }

    bool isValidTimestamp() const {
        return timestamp != std::chrono::system_clock::time_point();
    }

    friend std::ostream& operator<<(std::ostream& os, const LogEntry& entry) {
        os << "[" << entry.level_str << "] "
           << entry.raw_timestamp << " - "
           << entry.message;
        return os;
    }
};

#endif
