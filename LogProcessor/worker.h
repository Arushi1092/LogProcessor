#pragma once
#include <atomic>
#include <chrono>
#include "concurrentQueue.h"
#include "logentry.h"

// workerProcessor is the legacy single-entry-pop worker.
// The production path uses the inline lambda workers in main.cpp
// which use pop_bulk for 64x fewer lock acquisitions.
// This declaration is kept for the test project (LogprocessorTests).
void workerProcessor(
    ConcurrentQueue<LogEntry>& q,
    ConcurrentQueue<std::string>& writeQueue,
    std::atomic<int>& infoCount,
    std::atomic<int>& warnCount,
    std::atomic<int>& errorCount,
    const std::chrono::system_clock::time_point& filter_start,
    const std::chrono::system_clock::time_point& filter_end);