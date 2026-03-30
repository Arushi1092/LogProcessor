// timestamp_parser.h
#ifndef TIMESTAMP_PARSER_H
#define TIMESTAMP_PARSER_H

#include <string>
#include <chrono>
#include <ctime>
#include <regex>
#include <vector>
#include <sstream>

class TimestampParser {
public:
    struct TimestampFormat {
        std::regex pattern;
        std::string format;
        std::string description;
    };

private:
    std::vector<TimestampFormat> formats;

public:
    TimestampParser() {
        // Common log timestamp formats
        formats = {
            {std::regex(R"(\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2})"),
             "%Y-%m-%d %H:%M:%S", "ISO 8601"},

            {std::regex(R"(\d{4}/\d{2}/\d{2} \d{2}:\d{2}:\d{2})"),
             "%Y/%m/%d %H:%M:%S", "Slash separated"},

            {std::regex(R"(\d{2}/\d{2}/\d{4} \d{2}:\d{2}:\d{2})"),
             "%d/%m/%Y %H:%M:%S", "DD/MM/YYYY"},

            {std::regex(R"(\d{2}-\w{3}-\d{4} \d{2}:\d{2}:\d{2})"),
             "%d-%b-%Y %H:%M:%S", "DD-MMM-YYYY"},

            {std::regex(R"(\w{3} \d{2} \d{2}:\d{2}:\d{2})"),
             "%b %d %H:%M:%S", "MMM DD HH:MM:SS (no year)"}
        };
    }

    std::chrono::system_clock::time_point parse(const std::string& timestamp_str) {
        std::tm tm = {};

        for (const auto& format : formats) {
            std::smatch match;
            if (std::regex_search(timestamp_str, match, format.pattern)) {
                std::string extracted = match.str();
                std::istringstream ss(extracted);
                ss >> std::get_time(&tm, format.format.c_str());

                if (!ss.fail()) {
                    // Handle special cases
                    if (format.format == "%b %d %H:%M:%S") {
                        // Add current year if not present
                        auto now = std::chrono::system_clock::now();
                        auto now_t = std::chrono::system_clock::to_time_t(now);
                        std::tm* now_tm = std::localtime(&now_t);
                        tm.tm_year = now_tm->tm_year;
                    }

                    return std::chrono::system_clock::from_time_t(std::mktime(&tm));
                }
            }
        }

        // Return epoch if parsing fails
        return std::chrono::system_clock::time_point();
    }

    bool isWithinRange(const std::chrono::system_clock::time_point& tp,
        const std::chrono::system_clock::time_point& start,
        const std::chrono::system_clock::time_point& end) {
        return tp >= start && tp <= end;
    }
};

#endifs