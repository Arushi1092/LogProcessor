#pragma once
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <atomic>
#include <stdexcept>

class ThreadPool {
private:
    std::vector<std::thread>          workers;
    std::queue<std::function<void()>> taskQueue;
    std::mutex                        queueMutex;
    std::condition_variable           cv;
    std::atomic<bool>                 stop{ false };  // atomic: safe to read without lock

public:
    explicit ThreadPool(size_t numThreads) {
        workers.reserve(numThreads);
        for (size_t i = 0; i < numThreads; i++) {
            workers.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(queueMutex);
                        cv.wait(lock, [this] {
                            return stop.load() || !taskQueue.empty();
                            });
                        if (stop.load() && taskQueue.empty()) return;
                        task = std::move(taskQueue.front());
                        taskQueue.pop();
                    }   // lock released here — task runs in parallel
                    task();
                }
                });
        }
    }

    // Submit any callable — returns a future for the result
    template<typename F, typename... Args>
    auto submit(F&& f, Args&&... args)
        -> std::future<std::invoke_result_t<F, Args...>>
    {
        using ReturnType = std::invoke_result_t<F, Args...>;

        // Lambda capture instead of std::bind — avoids copies of move-only args
        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
            [f = std::forward<F>(f),
            args = std::make_tuple(std::forward<Args>(args)...)]() mutable {
                return std::apply(std::move(f), std::move(args));
            }
        );

        std::future<ReturnType> result = task->get_future();
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            if (stop.load())
                throw std::runtime_error("Cannot submit to stopped ThreadPool");
            taskQueue.emplace([task]() { (*task)(); });
        }
        cv.notify_one();
        return result;
    }

    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            stop.store(true);
        }
        cv.notify_all();
        for (auto& w : workers) w.join();
    }

    // Non-copyable, non-movable
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
};