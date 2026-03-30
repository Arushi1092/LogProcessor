// parser.h
#ifndef PARSER_H
#define PARSER_H

#include <string>
#include <chrono>
#include "logentry.h"

LogEntry parseLogLine(const std::string& line);
LogEntry parseLogLineWithTimeRange(const std::string& line,
    const std::chrono::system_clock::time_point& start_time,
    const std::chrono::system_clock::time_point& end_time);

#endif