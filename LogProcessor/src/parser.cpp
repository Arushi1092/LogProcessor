#include "parser.h"
#include "logentry.h"
#include <regex>
#include <string>
#include <chrono>

LogEntry parseLogLine(const std::string& line) {
    LogEntry entry;

    // matches: [INFO] message  or  [ERROR] message  etc.
    std::regex pattern(R"(\[(\w+)\]\s+(.+))");
    std::smatch match;

    if (std::regex_search(line, match, pattern)) {
        std::string lvl = match[1].str();
        std::string msg = match[2].str();

        auto now = std::chrono::system_clock::now();
        std::string ts = LogEntry::formatTimestamp(now);

        entry = LogEntry(lvl, msg, ts);
        entry.timestamp = now;
    }
    else {
        entry.level_str = "UNKNOWN";
        entry.level = LogLevel::UNKNOWN;
        entry.message = line;
        entry.raw_timestamp = "";
        entry.timestamp = std::chrono::system_clock::time_point();
    }

    return entry;
}