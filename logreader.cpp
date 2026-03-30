#include <iostream>
#include <fstream>
#include <string>
#include "concurrentQueue.h"
#include "logentry.h"
#include "parser.h"
#include "logreader.h"

using namespace std;

void logReader(const string& filename, ConcurrentQueue<LogEntry>& q) {

    ifstream file(filename);

    if (!file.is_open()) {
        cout << "Error: Cannot open file!" << endl;
        return;
    }

    string line;
    while (getline(file, line)) {
        if (line.empty()) continue;      // skip blank lines
        LogEntry entry = parseLogLine(line);
        q.push(entry);
    }

    file.close();
    cout << "Log reader finished." << endl;
}