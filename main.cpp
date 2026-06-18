#include <iostream>
#include <chrono>
#include <vector>
#include <future>
#include <mutex>
#include "thread_pool.hpp"

std::mutex cout_mutex;

int heavy_computation(int id, int matrix_size) {
    {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cout << "[TASK " << id << "] Started on thread ID: "
                  << std::this_thread::get_id() << std::endl;
    }

    long long accumulator = 0;
    for (int i = 0; i < matrix_size; ++i) {
        for (int j = 0; j < matrix_size; ++j) {
            accumulator += (i * j);
        }
    }
    (void)accumulator;

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cout << "[TASK " << id << "] Finished." << std::endl;
    }
    return id * 100;
}

int main() {
    std::cout << "=========================================================" << std::endl;
    std::cout << "   INITIALIZING ASYNCHRONOUS THREAD POOL ENGINE (C++)    " << std::endl;
    std::cout << "=========================================================" << std::endl;

    unsigned int hardware_cores = std::thread::hardware_concurrency();
    std::cout << "[SYSTEM] Detected hardware cores: " << hardware_cores << std::endl;

    ThreadPool pool(hardware_cores > 0 ? hardware_cores : 4);
    std::vector<std::future<int>> results;

    for (int i = 0; i < 8; ++i) {
        results.emplace_back(
            pool.enqueue(heavy_computation, i, 5000)
        );
    }

    std::cout << "\n[ENGINE] All tasks have been enqueued asynchronously.\n" << std::endl;

    for (size_t i = 0; i < results.size(); ++i) {
        int return_value = results[i].get();
        std::cout << "[RESULT] Task processed with return code: " << return_value << std::endl;
    }

    std::cout << "\n=========================================================" << std::endl;
    std::cout << "   ENGINE TEARDOWN COMPLETE - ALL THREADS CLEANED UP     " << std::endl;
    std::cout << "=========================================================" << std::endl;

    return 0;
}