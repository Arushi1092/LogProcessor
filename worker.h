#ifndef WORKER_H
#define WORKER_H

#include <atomic>
#include <chrono>
#include "concurrentQueue.h"
#include "logentry.h"

// Function declarations
// Old worker-style processor with time range filtering. Rename to avoid
// collision with the main logProcessor that writes to an ostream.
void workerProcessor(ConcurrentQueue<LogEntry>& q,
    std::atomic<int>& infoCount,
    std::atomic<int>& warnCount,
    std::atomic<int>& errorCount,
    const std::chrono::system_clock::time_point& filter_start = {},
    const std::chrono::system_clock::time_point& filter_end = {});

#endif