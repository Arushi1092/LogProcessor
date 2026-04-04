#pragma once
#include <string>
#include "concurrentQueue.h"

void logReader(const std::string& filename, ConcurrentQueue<std::string>& q);