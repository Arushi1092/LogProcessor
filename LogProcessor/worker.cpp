#include "worker.h"
#include "concurrentQueue.h"
#include "logentry.h"
#include "parser.h"
#include <atomic>
#include <chrono>
#include <string>

// Single-entry-pop worker — kept for the test project.
// Production code uses pop_bulk workers defined in main.cpp.
void workerProcessor(
    ConcurrentQueue<LogEntry>& q,
    ConcurrentQueue<std::string>& writeQueue,
    std::atomic<int>& infoCount,
    std::atomic<int>& warnCount,
    std::atomic<int>& errorCount,
    const std::chrono::system_clock::time_point& filter_start,
    const std::chrono::system_clock::time_point& filter_end)
{
    bool filter_enabled = (
        filter_start != std::chrono::system_clock::time_point{} ||
        filter_end != std::chrono::system_clock::time_point{});

    LogEntry entry;
    while (q.pop(entry)) {
        if (filter_enabled && entry.isValidTimestamp()) {
            if (filter_start != std::chrono::system_clock::time_point{} &&
                entry.timestamp < filter_start) continue;
            if (filter_end != std::chrono::system_clock::time_point{} &&
                entry.timestamp > filter_end)   continue;
        }

        switch (entry.level) {
        case LogLevel::INFO:  infoCount++;  break;
        case LogLevel::WARN:  warnCount++;  break;
        case LogLevel::ERROR: errorCount++; break;
        default: break;
        }

        std::string line;
        switch (entry.level) {
        case LogLevel::INFO:
            line = "[INFO]  " + entry.raw_timestamp + " - " + entry.message + "\n";
            break;
        case LogLevel::WARN:
            line = "[WARN]  " + entry.raw_timestamp + " - " + entry.message + "\n";
            break;
        case LogLevel::ERROR:
            line = "[ERROR] " + entry.raw_timestamp + " - " + entry.message + "\n";
            break;
        default:
            line = "[UNKNOWN] " + entry.raw_timestamp + " - " + entry.message + "\n";
            break;
        }
        writeQueue.push(std::move(line));
    }
}