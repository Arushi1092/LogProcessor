#include <iostream>
#include <ostream>
#include <atomic>
#include <mutex>
#include "concurrentQueue.h"
#include "logentry.h"
#include "logprocessor.h"

using namespace std;

mutex outputMutex;

void logProcessor(ConcurrentQueue<LogEntry>& q, int threadId,
    atomic<int>& errorCount,
    atomic<int>& warnCount,
    atomic<int>& infoCount,
    ostream& out)
{
    LogEntry entry;

    while (q.pop(entry)) {

        // update counters
        if (entry.level == LogLevel::ERROR) ++errorCount;
        else if (entry.level == LogLevel::WARN)  ++warnCount;
        else if (entry.level == LogLevel::INFO)  ++infoCount;

        // print with level_str and timestamp
        {
            lock_guard<mutex> lock(outputMutex);
            out << "[Thread " << threadId << "] "
                << entry.level_str << " - "
                << entry.raw_timestamp << " - "
                << entry.message << "\n";
        }
    }

    {
        lock_guard<mutex> lock(outputMutex);
        out << "[Thread " << threadId << "] done.\n";
    }
}