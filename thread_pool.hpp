#ifndef THREAD_POOL_HPP
#define THREAD_POOL_HPP

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <memory>
#include <stdexcept>
#include <utility>
#include <tuple>

/**
 * @brief Un pool de hilos de alto rendimiento que implementa una cola de tareas.
 */
class ThreadPool {
public:
    explicit ThreadPool(size_t threads);

    template<class F, class... Args>
    [[nodiscard]] auto enqueue(F&& f, Args&&... args)
        -> std::future<std::invoke_result_t<F, Args...>>;

    ~ThreadPool() noexcept;

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queue_mutex;
    std::condition_variable cv;
    bool stop;
};

inline ThreadPool::ThreadPool(size_t threads) : stop(false) {
    for (size_t i = 0; i < threads; ++i) {
        workers.emplace_back([this] {
            while (true) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(queue_mutex);
                    cv.wait(lock, [this] {
                        return stop || !tasks.empty();
                    });
                    if (stop && tasks.empty())
                        return;
                    task = std::move(tasks.front());
                    tasks.pop();
                }
                task();
            }
        });
    }
}

template<class F, class... Args>
[[nodiscard]] auto ThreadPool::enqueue(F&& f, Args&&... args)
    -> std::future<std::invoke_result_t<F, Args...>> {
    using return_type = std::invoke_result_t<F, Args...>;

    // Empaquetar argumentos en una tupla para evitar pack expansion en C++17
    auto args_tuple = std::make_tuple(std::forward<Args>(args)...);
    auto task = std::make_shared<std::packaged_task<return_type()>>(
        [f = std::forward<F>(f), args = std::move(args_tuple)]() mutable {
            return std::apply(std::move(f), std::move(args));
        }
    );

    std::future<return_type> result = task->get_future();

    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        if (stop) {
            throw std::runtime_error("enqueue on stopped ThreadPool");
        }
        tasks.emplace([task = std::move(task)]() { (*task)(); });
    }
    cv.notify_one();
    return result;
}

inline ThreadPool::~ThreadPool() noexcept {
    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        stop = true;
    }
    cv.notify_all();
    for (std::thread& worker : workers) {
        if (worker.joinable())
            worker.join();
    }
}

#endif // THREAD_POOL_HPP