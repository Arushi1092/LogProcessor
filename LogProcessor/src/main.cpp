#include <iostream>
#include <fstream>
#include <thread>
#include <string>
#include <atomic>
#include <vector>
#include <chrono>
#include <iomanip>
#include "concurrentQueue.h"
#include "logEntry.h"
#include "logreader.h"
#include "logprocessor.h"
#include "threadPool.h"

using namespace std;
using namespace std::chrono;

// discards all output with no mutex cost
struct NullStream : ostream {
    NullStream() : ostream(nullptr) {}
};

double runBenchmark(const string& filename, int numThreads, bool showOutput = false) {
    ConcurrentQueue<LogEntry> q;
    atomic<int> errorCount{ 0 }, warnCount{ 0 }, infoCount{ 0 };

    NullStream nullStream;
    ostream& out = showOutput
        ? static_cast<ostream&>(cout)
        : static_cast<ostream&>(nullStream);

    auto start = high_resolution_clock::now();

    thread reader(logReader, filename, ref(q));

    // use thread pool instead of raw threads
    ThreadPool pool(numThreads);
    vector<future<void>> futures;

    for (int i = 1; i <= numThreads; i++) {
        futures.push_back(pool.submit([&, i]() {
            logProcessor(q, i, errorCount, warnCount, infoCount, out);
            }));
    }

    reader.join();
    q.shutdown();

    for (auto& f : futures) f.get();

    auto end = high_resolution_clock::now();

    return duration<double, milli>(end - start).count();
}

int main() {

    // ---- normal run on sample.log ----
    ConcurrentQueue<LogEntry> q;
    atomic<int> errorCount{ 0 }, warnCount{ 0 }, infoCount{ 0 };

    ofstream outFile("data/results.txt");
    if (!outFile.is_open()) {
        cerr << "Error: Cannot open output file!\n";
        return 1;
    }

    string filename = "data/sample.log";

    cout << "=== Log Processing System ===\n";
    cout << "Reading from: " << filename << "\n";
    cout << "Writing to  : data/results.txt\n";
    cout << "Starting 3 consumer threads...\n";
    cout << "==============================\n";
    outFile << "=== Log Processing System ===\n";
    outFile << "Reading from: " << filename << "\n";
    outFile << "==============================\n";

    thread reader(logReader, filename, ref(q));

    ThreadPool pool(3);   // 3 workers, created once

    // submit tasks to pool instead of creating threads
    auto f1 = pool.submit([&]() {
        logProcessor(q, 1, errorCount, warnCount, infoCount, outFile);
        });
    auto f2 = pool.submit([&]() {
        logProcessor(q, 2, errorCount, warnCount, infoCount, outFile);
        });
    auto f3 = pool.submit([&]() {
        logProcessor(q, 3, errorCount, warnCount, infoCount, outFile);
        });

    reader.join();
    q.shutdown();

    // wait for all tasks to complete
    f1.get();
    f2.get();
    f3.get();


    int total = infoCount + warnCount + errorCount;
    string summary =
        "\n=== Processing Complete ===\n"
        "Total INFO  : " + to_string(infoCount) + "\n" +
        "Total WARN  : " + to_string(warnCount) + "\n" +
        "Total ERROR : " + to_string(errorCount) + "\n" +
        "Total lines : " + to_string(total) + "\n" +
        "===========================\n";

    cout << summary;
    outFile << summary;
    outFile.close();
    cout << "Results saved to data/results.txt\n";

    // ---- benchmark ----
    cout << "\n=== Benchmark ===\n";
    cout << "Generating 100,000 line test file...\n";

    ofstream bigLog("data/biglog.txt");
    for(int i= 0;i < 100000;i++) {
        if(i%10 == 0) bigLog << "[ERROR] Something went wrong at line " << i << "\n";
        else if (i%5== 0) bigLog << "[WARN]  High memory usage at line " << i << "\n";
        else                  bigLog << "[INFO]  Processing request at line " << i << "\n";
    }
    bigLog.close();
    cout << "Done. Running benchmarks (3 runs each)...\n\n";

    cout << "--- Logs from 1-thread run ---\n";
    runBenchmark("data/biglog.txt", 1, true);   // show logs once
    cout << "--- End of logs ---\n\n";

    for (int n : {1, 2, 3, 5, 8}) {
        double total = 0;
        for (int r = 0; r < 3; r++)
            total += runBenchmark("data/biglog.txt", n, false);  // silent runs
        cout << n << " thread(s) : "
            << fixed << setprecision(2)
            << (total / 3.0) << " ms\n";
    }

    cout << "\n=================\n";
    return 0;
}