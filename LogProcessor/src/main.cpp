#include <iostream>
#include <fstream>
#include <thread>
#include <string>
#include <atomic>
#include <vector>
#include <future>
#include <chrono>
#include <iomanip>
#include <filesystem>
#include "concurrentQueue.h"
#include "logentry.h"
#include "parser.h"
#include "logreader.h"
#include "threadPool.h"

using namespace std;
using namespace std::chrono;

auto makeWorker(
    ConcurrentQueue<string>& q,
    ConcurrentQueue<string>& writeQueue,
    atomic<int>& infoCount,
    atomic<int>& warnCount,
    atomic<int>& errorCount,
    system_clock::time_point filter_start,
    system_clock::time_point filter_end,
    bool filter_enabled)
{
    return [&q, &writeQueue, &infoCount, &warnCount, &errorCount,
        filter_start, filter_end, filter_enabled]()
        {
            vector<string> batch;    batch.reserve(64);
            vector<string> outBatch; outBatch.reserve(64);

            auto flushOut = [&]() {
                if (!outBatch.empty()) {
                    writeQueue.push_bulk(outBatch);
                    outBatch.clear();
                }
                };

            while (q.pop_bulk(batch, 64)) {
                for (auto& raw : batch) {
                    LogEntry entry = parseLogLine(raw);

                    if (filter_enabled && entry.isValidTimestamp()) {
                        if (filter_start != system_clock::time_point{} &&
                            entry.timestamp < filter_start) continue;
                        if (filter_end != system_clock::time_point{} &&
                            entry.timestamp > filter_end) continue;
                    }

                    if (entry.level == LogLevel::INFO)  infoCount++;
                    else if (entry.level == LogLevel::WARN)  warnCount++;
                    else if (entry.level == LogLevel::ERROR) errorCount++;

                    outBatch.push_back("[" + entry.level_str + "] "
                        + entry.raw_timestamp + " - "
                        + entry.message + "\n");

                    if (outBatch.size() >= 64) flushOut();
                }
                batch.clear();
            }
            flushOut();
        };
}

double runBenchmark(const string& filename, int numThreads, bool showOutput = false) {
    ConcurrentQueue<string> q;
    ConcurrentQueue<string> writeQueue;
    atomic<int>       errorCount{ 0 }, warnCount{ 0 }, infoCount{ 0 };
    atomic<long long> readerMs{ 0 }, workerMs{ 0 }, writerMs{ 0 };

    auto filter_start = system_clock::time_point{};
    auto filter_end = system_clock::time_point{};
    bool filter_enabled = false;

    auto wallStart = high_resolution_clock::now();

    thread reader([&]() {
        auto t1 = high_resolution_clock::now();
        logReader(filename, q);
        readerMs = duration_cast<milliseconds>(
            high_resolution_clock::now() - t1).count();
        });

    thread writer([&]() {
        auto t1 = high_resolution_clock::now();
        string line;
        while (writeQueue.pop(line)) {
            if (showOutput) cout << line;
        }
        writerMs = duration_cast<milliseconds>(
            high_resolution_clock::now() - t1).count();
        });

    {
        ThreadPool pool(numThreads);
        vector<future<void>> futures;
        futures.reserve(numThreads);

        for (int i = 0; i < numThreads; i++) {
            futures.push_back(pool.submit(
                [&q, &writeQueue, &infoCount, &warnCount, &errorCount,
                &workerMs, filter_start, filter_end, filter_enabled]()
                {
                    auto t1 = high_resolution_clock::now();

                    vector<string> batch;    batch.reserve(64);
                    vector<string> outBatch; outBatch.reserve(64);

                    auto flushOut = [&]() {
                        if (!outBatch.empty()) {
                            writeQueue.push_bulk(outBatch);
                            outBatch.clear();
                        }
                        };

                    while (q.pop_bulk(batch, 64)) {
                        for (auto& raw : batch) {
                            LogEntry entry = parseLogLine(raw);

                            if (filter_enabled && entry.isValidTimestamp()) {
                                if (filter_start != system_clock::time_point{} &&
                                    entry.timestamp < filter_start) continue;
                                if (filter_end != system_clock::time_point{} &&
                                    entry.timestamp > filter_end) continue;
                            }

                            if (entry.level == LogLevel::INFO)  infoCount++;
                            else if (entry.level == LogLevel::WARN)  warnCount++;
                            else if (entry.level == LogLevel::ERROR) errorCount++;

                            outBatch.push_back("[" + entry.level_str + "] "
                                + entry.raw_timestamp + " - "
                                + entry.message + "\n");

                            if (outBatch.size() >= 64) flushOut();
                        }
                        batch.clear();
                    }
                    flushOut();

                    workerMs += duration_cast<milliseconds>(
                        high_resolution_clock::now() - t1).count();
                }
            ));
        }

        reader.join();
        q.shutdown();
        for (auto& f : futures) f.get();
    }

    writeQueue.shutdown();
    writer.join();

    double total = duration<double, milli>(
        high_resolution_clock::now() - wallStart).count();

    cout << "  [n=" << numThreads << "] "
        << "reader=" << readerMs.load() << "ms  "
        << "workers=" << workerMs.load() << "ms  "
        << "writer=" << writerMs.load() << "ms  "
        << "total=" << fixed << setprecision(1) << total << "ms\n";

    return total;
}

int main() {
    filesystem::create_directories("data");

    ConcurrentQueue<string> q;
    ConcurrentQueue<string> writeQueue;
    atomic<int> errorCount{ 0 }, warnCount{ 0 }, infoCount{ 0 };

    ofstream outFile("data/results.txt");
    if (!outFile.is_open()) {
        cerr << "Error: Cannot open data/results.txt\n";
        return 1;
    }

    const string filename = "data/sample.log";
    auto filter_start = system_clock::time_point{};
    auto filter_end = system_clock::time_point{};
    bool filter_enabled = false;

    cout << "=== Log Processing System ===\n"
        << "Reading from : " << filename << "\n"
        << "Writing to   : data/results.txt\n"
        << "Threads      : 3 workers + 1 dedicated writer\n"
        << "Filtering    : " << (filter_enabled ? "enabled" : "disabled") << "\n"
        << "==============================\n";

    outFile << "=== Log Processing System ===\n"
        << "Reading from : " << filename << "\n"
        << "==============================\n";

    thread reader(logReader, filename, ref(q));
    thread writer([&]() {
        string line;
        while (writeQueue.pop(line)) outFile << line;
        });

    {
        ThreadPool pool(3);
        auto f1 = pool.submit(makeWorker(q, writeQueue, infoCount, warnCount, errorCount,
            filter_start, filter_end, filter_enabled));
        auto f2 = pool.submit(makeWorker(q, writeQueue, infoCount, warnCount, errorCount,
            filter_start, filter_end, filter_enabled));
        auto f3 = pool.submit(makeWorker(q, writeQueue, infoCount, warnCount, errorCount,
            filter_start, filter_end, filter_enabled));
        reader.join();
        q.shutdown();
        f1.get(); f2.get(); f3.get();
    }

    writeQueue.shutdown();
    writer.join();

    const string summary =
        "\n=== Processing Complete ===\n"
        "Total INFO  : " + to_string(infoCount.load()) + "\n" +
        "Total WARN  : " + to_string(warnCount.load()) + "\n" +
        "Total ERROR : " + to_string(errorCount.load()) + "\n" +
        "Total lines : " + to_string(infoCount + warnCount + errorCount) + "\n" +
        (filter_enabled ? "Filtering   : active\n" : "") +
        "===========================\n";

    cout << summary;
    outFile << summary;
    outFile.close();
    cout << "Results saved to data/results.txt\n";

    // ---- Benchmark ----
    cout << "\n=== Benchmark ===\n"
        << "Generating 500,000 line test file...\n";

    {
        ofstream bigLog("data/biglog.txt");
        if (!bigLog.is_open()) {
            cerr << "Error: Cannot create data/biglog.txt\n";
            return 1;
        }

        const char* dates[] = {
            "2026-01-15 08:23:11",
            "2026-02-20 13:47:55",
            "2026-03-10 09:05:32",
            "2026-04-03 16:30:24",
            "2026-04-03 22:11:08"
        };

        for (int i = 0; i < 500000; i++) {
            const char* ts = dates[i % 5];
            if (i % 10 == 0)
                bigLog << "[ERROR] " << ts
                << " - {\"event\":\"failure\",\"code\":" << (i % 500)
                << ",\"service\":\"auth\",\"latency_ms\":" << (50 + i % 200)
                << ",\"user\":\"user_" << i << "\"}\n";
            else if (i % 5 == 0)
                bigLog << "[WARN]  " << ts
                << " - {\"event\":\"high_memory\",\"current_mb\":"
                << (512 + i % 256) << ",\"host\":\"node-" << (i % 16) << "\"}\n";
            else
                bigLog << "[INFO]  " << ts
                << " - {\"event\":\"http_request\",\"path\":\"/api/v2/resource/"
                << i << "\",\"status\":200,\"duration_ms\":" << (i % 80) << "}\n";
        }
    }

    cout << "Done. Running benchmarks (3 runs each)...\n\n";

    for (int n : {1, 2, 3, 4, 8}) {
        double totalMs = 0;
        for (int r = 0; r < 3; r++)
            totalMs += runBenchmark("data/biglog.txt", n, false);
        cout << n << " thread(s) avg: "
            << fixed << setprecision(1)
            << (totalMs / 3.0) << " ms\n\n";
    }

    cout << "=================\n";
    return 0;
}