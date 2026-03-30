#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>

template<typename T>
class ConcurrentQueue {
private:
    std::queue<T> q;
    std::mutex m;
    std::condition_variable cv;
    bool stop = false;

public:
    void push(T value) {
        std::unique_lock<std::mutex> lock(m);
        q.push(value);
        lock.unlock();
        cv.notify_one();
    }

    bool pop(T& value) {
        std::unique_lock<std::mutex> lock(m);

        cv.wait(lock, [this] {
            return !q.empty() || stop;
            });

        if (stop && q.empty()) return false;

        value = q.front();
        q.pop();

        return true;
    }

    void shutdown() {
        std::unique_lock<std::mutex> lock(m);
        stop = true;
        lock.unlock();
        cv.notify_all();
    }
};