#include "worker.h"
#include "concurrentQueue.h"
#include "logentry.h"
#include <atomic>
#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>

// Global mutex for thread-safe console output
static std::mutex coutMutex;

void workerProcessor(ConcurrentQueue<LogEntry>& q,
    std::atomic<int>& infoCount,
    std::atomic<int>& warnCount,
    std::atomic<int>& errorCount,
    const std::chrono::system_clock::time_point& filter_start,
    const std::chrono::system_clock::time_point& filter_end) {

    LogEntry entry;
    int processed = 0;

    // Check if filtering is enabled
    bool filter_enabled = (filter_start != std::chrono::system_clock::time_point{} ||
        filter_end != std::chrono::system_clock::time_point{});

    while (true) {
        bool success = q.pop(entry);

        if (!success) {
            {
                std::lock_guard<std::mutex> lock(coutMutex);
                std::cout << "[Thread " << std::this_thread::get_id()
                    << "] Processed " << processed << " entries, exiting" << std::endl;
            }
            break;
        }

        // Apply timestamp filter if enabled
        if (filter_enabled) {
            if (!entry.isValidTimestamp()) {
                continue;
            }

            if (filter_start != std::chrono::system_clock::time_point{} &&
                entry.timestamp < filter_start) {
                continue;
            }

            if (filter_end != std::chrono::system_clock::time_point{} &&
                entry.timestamp > filter_end) {
                continue;
            }
        }

        processed++;

        // Process based on log level using the enum
        switch (entry.level) {
        case LogLevel::INFO:
            infoCount++;
            {
                std::lock_guard<std::mutex> lock(coutMutex);
                std::cout << "[INFO] " << entry.raw_timestamp
                    << " - " << entry.message << std::endl;
            }
            break;

        case LogLevel::WARN:
            warnCount++;
            {
                std::lock_guard<std::mutex> lock(coutMutex);
                std::cout << "[WARN] " << entry.raw_timestamp
                    << " - " << entry.message << std::endl;
            }
            break;

        case LogLevel::ERROR:
            errorCount++;
            {
                std::lock_guard<std::mutex> lock(coutMutex);
                std::cout << "[ERROR] " << entry.raw_timestamp
                    << " - " << entry.message << std::endl;
            }
            break;

        default:
        {
            std::lock_guard<std::mutex> lock(coutMutex);
            std::cout << "[UNKNOWN] " << entry.raw_timestamp
                << " - " << entry.message << std::endl;
        }
        break;
        }
    }
}