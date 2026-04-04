#pragma once
#include <atomic>
#include <ostream>
#include "concurrentQueue.h"
#include "logentry.h"

// logProcessor is retired — logprocessor.h now only exposes
// the queue aliases used by main and the orchestration helpers.
// No mutex, no direct I/O from worker threads.