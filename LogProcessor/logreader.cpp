#include "logreader.h"
#include "concurrentQueue.h"
#include <windows.h>
#include <string>
#include <string_view>
#include <vector>
#include <iostream>

void logReader(const std::string& filename, ConcurrentQueue<std::string>& q) {
    HANDLE hFile = CreateFileA(
        filename.c_str(), GENERIC_READ, FILE_SHARE_READ,
        nullptr, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, nullptr);

    if (hFile == INVALID_HANDLE_VALUE) {
        std::cerr << "logReader: cannot open " << filename << "\n";
        q.shutdown();
        return;
    }

    LARGE_INTEGER size{};
    GetFileSizeEx(hFile, &size);

    if (size.QuadPart == 0) {
        CloseHandle(hFile);
        q.shutdown();
        return;
    }

    HANDLE hMap = CreateFileMappingA(hFile, nullptr, PAGE_READONLY, 0, 0, nullptr);
    if (!hMap) {
        CloseHandle(hFile);
        q.shutdown();
        return;
    }

    const char* data = static_cast<const char*>(
        MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0));

    if (!data) {
        CloseHandle(hMap);
        CloseHandle(hFile);
        q.shutdown();
        return;
    }

    std::string_view view(data, static_cast<size_t>(size.QuadPart));

    std::vector<std::string> batch;
    batch.reserve(512);

    size_t start = 0;
    for (size_t i = 0; i <= view.size(); ++i) {
        if (i == view.size() || view[i] == '\n') {
            if (i > start) {
                size_t end = i;
                if (end > start && view[end - 1] == '\r') --end;
                if (end > start)
                    batch.emplace_back(view.data() + start, end - start);
            }
            start = i + 1;
            if (batch.size() >= 512) {
                q.push_bulk(batch);
                batch.clear();
            }
        }
    }

    if (!batch.empty())
        q.push_bulk(batch);

    UnmapViewOfFile(data);
    CloseHandle(hMap);
    CloseHandle(hFile);

    std::cout << "Log reader finished.\n";
}