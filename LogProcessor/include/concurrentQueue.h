#pragma once
#include <queue>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <atomic>

template<typename T>
class ConcurrentQueue {
private:
    std::queue<T>           q;
    std::mutex              m;
    std::condition_variable cv_pop;
    std::condition_variable cv_push;
    std::atomic<bool>       stop{ false };
    static constexpr size_t MAX_SIZE = 16384;

public:
    // Single push
    void push(T value) {
        {
            std::unique_lock<std::mutex> lock(m);
            cv_push.wait(lock, [this] {
                return q.size() < MAX_SIZE || stop.load();
                });
            if (stop.load()) return;
            q.push(std::move(value));
        }
        cv_pop.notify_one();
    }

    // Bulk push — releases and reacquires lock between batches so the
    // consumer can drain when the queue is near-full (prevents deadlock
    // under backpressure — the previous version held the lock in the loop)
    void push_bulk(std::vector<T>& items) {
        size_t i = 0;
        while (i < items.size()) {
            {
                std::unique_lock<std::mutex> lock(m);
                cv_push.wait(lock, [this] {
                    return q.size() < MAX_SIZE || stop.load();
                    });
                if (stop.load()) return;

                // Push as many as fit without re-waiting
                while (i < items.size() && q.size() < MAX_SIZE)
                    q.push(std::move(items[i++]));
            }   // lock released — consumer can drain before next batch
            cv_pop.notify_all();
        }
    }

    // Single pop — blocks until item available or shutdown
    bool pop(T& value) {
        std::unique_lock<std::mutex> lock(m);
        cv_pop.wait(lock, [this] {
            return !q.empty() || stop.load();
            });
        if (stop.load() && q.empty()) return false;
        value = std::move(q.front());
        q.pop();
        cv_push.notify_one();
        return true;
    }

    // Bulk pop — one lock acquisition for up to max_items
    // Wakes exactly as many producers as slots freed (not notify_all)
    bool pop_bulk(std::vector<T>& out, size_t max_items) {
        std::unique_lock<std::mutex> lock(m);
        cv_pop.wait(lock, [this] {
            return !q.empty() || stop.load();
            });
        if (stop.load() && q.empty()) return false;

        size_t popped = 0;
        while (!q.empty() && out.size() < max_items) {
            out.push_back(std::move(q.front()));
            q.pop();
            ++popped;
        }
        lock.unlock();  // release before notifying
        for (size_t i = 0; i < popped; ++i)
            cv_push.notify_one();   // wake one producer per freed slot
        return true;
    }

    void shutdown() {
        stop.store(true);
        cv_pop.notify_all();
        cv_push.notify_all();
    }

    // Non-copyable
    ConcurrentQueue() = default;
    ConcurrentQueue(const ConcurrentQueue&) = delete;
    ConcurrentQueue& operator=(const ConcurrentQueue&) = delete;
};