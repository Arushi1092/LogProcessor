
#pragma once
#include <string>
#include "concurrentQueue.h"
#include "logentry.h"

// Declare logReader to match implementation: takes filename by const ref and
// pushes LogEntry objects into the provided queue.
void logReader(const std::string& filename, ConcurrentQueue<LogEntry>& q);
