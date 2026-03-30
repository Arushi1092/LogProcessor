#pragma once
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <stdexcept>

class ThreadPool {
private:
    std::vector<std::thread>          workers;
    std::queue<std::function<void()>> taskQueue;
    std::mutex                        queueMutex;
    std::condition_variable           cv;
    bool                              stop = false;

public:
    // constructor — spawns numThreads workers immediately
    ThreadPool(size_t numThreads) {
        for (size_t i = 0; i < numThreads; i++) {
            workers.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(queueMutex);
                        cv.wait(lock, [this] {
                            return stop || !taskQueue.empty();
                            });
                        if (stop && taskQueue.empty()) return;
                        task = std::move(taskQueue.front());
                        taskQueue.pop();
                    }
                    task();   // execute outside the lock
                }
                });
        }
    }

    // submit any callable — returns a future so caller can wait for result
    template<typename F, typename... Args>
    auto submit(F&& f, Args&&... args)
        -> std::future<std::invoke_result_t<F, Args...>>
    {
        using ReturnType = std::invoke_result_t<F, Args...>;

        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

        std::future<ReturnType> result = task->get_future();

        {
            std::unique_lock<std::mutex> lock(queueMutex);
            if (stop)
                throw std::runtime_error("Cannot submit to stopped ThreadPool");
            taskQueue.emplace([task]() { (*task)(); });
        }
        cv.notify_one();
        return result;
    }

    // destructor — shuts down cleanly, waits for all tasks to finish
    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            stop = true;
        }
        cv.notify_all();
        for (auto& worker : workers)
            worker.join();
    }
};
