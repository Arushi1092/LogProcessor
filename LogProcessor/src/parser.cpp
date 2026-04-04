#include "parser.h"
#include "logentry.h"
#include <string>
#include <chrono>
#include <ctime>
#include <cstdio>

LogEntry parseLogLine(const std::string& line) {
    LogEntry entry;

    if (line.size() < 3 || line[0] != '[') {
        entry.level_str     = "UNKNOWN";
        entry.level         = LogLevel::UNKNOWN;
        entry.message       = line;
        entry.raw_timestamp = "";
        entry.timestamp     = std::chrono::system_clock::time_point{};
        return entry;
    }

    // Parse level from [LEVEL]
    auto close = line.find(']');
    if (close == std::string::npos || close + 1 >= line.size()) {
        entry.level_str     = "UNKNOWN";
        entry.level         = LogLevel::UNKNOWN;
        entry.message       = line;
        entry.raw_timestamp = "";
        entry.timestamp     = std::chrono::system_clock::time_point{};
        return entry;
    }

    entry.level_str = line.substr(1, close - 1);
    entry.level     = stringToLogLevel(entry.level_str);

    // Skip past ']' and spaces
    size_t pos = close + 1;
    while (pos < line.size() && line[pos] == ' ') ++pos;

    // Try to parse "YYYY-MM-DD HH:MM:SS - message"
    // sscanf_s is 5-10x faster than istringstream + get_time
    int Y, M, D, h, m, s;
    int matched = sscanf_s(line.c_str() + pos,
                           "%d-%d-%d %d:%d:%d",
                           &Y, &M, &D, &h, &m, &s);

    if (matched == 6 && pos + 19 < line.size()) {
        entry.raw_timestamp = line.substr(pos, 19);

        std::tm tm{};
        tm.tm_year = Y - 1900;
        tm.tm_mon  = M - 1;
        tm.tm_mday = D;
        tm.tm_hour = h;
        tm.tm_min  = m;
        tm.tm_sec  = s;
        tm.tm_isdst = -1;

#ifdef _WIN32
        std::time_t t = _mkgmtime(&tm);
#else
        std::time_t t = timegm(&tm);
#endif
        entry.timestamp = (t != -1)
            ? std::chrono::system_clock::from_time_t(t)
            : std::chrono::system_clock::time_point{};

        // Skip past "YYYY-MM-DD HH:MM:SS - "
        size_t sep = pos + 19;
        while (sep < line.size() && (line[sep] == ' ' || line[sep] == '-')) ++sep;
        while (sep < line.size() &&  line[sep] == ' ') ++sep;
        entry.message = line.substr(sep);
    } else {
        // No timestamp — treat rest of line as message
        entry.message       = line.substr(pos);
        entry.raw_timestamp = "";
        entry.timestamp     = std::chrono::system_clock::time_point{};
    }

    return entry;
}