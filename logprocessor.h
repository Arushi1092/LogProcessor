#pragma once
#include <atomic>
#include "concurrentQueue.h"
#include "logentry.h"

// Process LogEntry objects from the queue. Writes output to the provided
// ostream (can be an ofstream or cout). This signature matches calls in
// main.cpp and the benchmark harness.
void logProcessor(ConcurrentQueue<LogEntry>& q, int threadId,
    std::atomic<int>& errorCount,
    std::atomic<int>& warnCount,
    std::atomic<int>& infoCount,
    std::ostream& out);
