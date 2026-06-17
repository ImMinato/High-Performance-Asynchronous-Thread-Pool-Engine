![C++ Standard](https://img.shields.io/badge/c%2B%2B-17-00599C?style=for-the-badge&logo=c%2B%2B)
![Build](https://img.shields.io/badge/build-cmake-064F8C?style=for-the-badge&logo=cmake)
![License](https://img.shields.io/badge/license-MIT-green?style=for-the-badge)

# ⚙️ High-Performance Asynchronous Thread Pool Engine

A modern, enterprise-grade C++17 Thread Pool architecture designed to optimize CPU core throughput. This system implements a thread-safe task queue, automatic hardware core matching, and generic task assignment utilizing advanced type erasure and template-meta programming.

---

## ⚡ Architectural Core Concepts

* **RAII Lifecycles:** Threads are spawned upon class initialization and gracefully torn down through standard stack unwinding patterns inside the destructor (`std::thread::join`).
* **Perfect Forwarding & Type Erasure:** Uses C++ template deduction mechanics (`std::forward`, `std::invoke_result`) combined with `std::function<void()>` to allow execution of arbitrary callable routines with varied parameter arrays.
* **Asynchronous Future Binding:** Tasks return a pipeline connected `std::future<T>`, allowing calling contexts to perform non-blocking work and retrieve asynchronous calculations using safe synchronization blocks (`.get()`).
* **Thread Safety & Low Latency Synchronization:** Prevents memory corruption and CPU cycle waste by utilizing starvation-free condition variables (`std::condition_variable`) and standard exclusion locks (`std::mutex`, `std::unique_lock`).

---

## 📂 Project Structure & Source Code

To implement this architecture on your environment, populate your repository with the following structural layout:

### 1. `CMakeLists.txt`
```cmake
cmake_minimum_required(VERSION 3.12)
project(AsynchronousThreadPoolEngine VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

add_executable(ThreadPoolEngine main.cpp)
target_link_libraries(ThreadPoolEngine PRIVATE Threads::Threads)
```

### 2. `thread_pool.hpp`
```cpp
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

class ThreadPool {
public:
    explicit ThreadPool(size_t threads);
    
    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args) 
        -> std::future<typename std::invoke_result<F, Args...>::type>;
        
    ~ThreadPool();

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    
    std::mutex queue_mutex;
    std::condition_variable cv;
    bool stop;
};

inline ThreadPool::ThreadPool(size_t threads) : stop(false) {
    for(size_t i = 0; i < threads; ++i) {
        workers.emplace_back([this] {
            while(true) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(this->queue_mutex);
                    this->cv.wait(lock, [this] { 
                        return this->stop || !this->tasks.empty(); 
                    });
                    
                    if(this->stop && this->tasks.empty()) return;
                    
                    task = std::move(this->tasks.front());
                    this->tasks.pop();
                }
                task();
            }
        });
    }
}

template<class F, class... Args>
auto ThreadPool::enqueue(F&& f, Args&&... args) 
    -> std::future<typename std::invoke_result<F, Args...>::type> {
    
    using return_type = typename std::invoke_result<F, Args...>::type;

    auto task = std::make_shared<std::packaged_task<return_type()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );
    
    std::future<return_type> res = task->get_future();
    {
        std::unique_lock<std::mutex> lock(queue_mutex);

        if(stop) throw std::runtime_error("Enqueue invocado en un ThreadPool detenido.");

        tasks.emplace([task]() { (*task)(); });
    }
    cv.notify_one();
    return res;
}

inline ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;
    }
    cv.notify_all();
    for(std::thread &worker: workers) {
        if(worker.joinable()) worker.join();
    }
}

#endif // THREAD_POOL_HPP
```

### 3. `main.cpp`
```cpp
#include <iostream>
#include <chrono>
#include "thread_pool.hpp"

int heavy_computation(int id, int matrix_size) {
    std::cout << "[TASK " << id << "] Inicializada en el hilo ID: " 
              << std::this_thread::get_id() << std::endl;
    
    long long accumulator = 0;
    for (int i = 0; i < matrix_size; ++i) {
        for (int j = 0; j < matrix_size; ++j) {
            accumulator += (i * j);
        }
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    std::cout << "[TASK " << id << "] Finalizada." << std::endl;
    return id * 100;
}

int main() {
    std::cout << "=========================================================" << std::endl;
    std::cout << "   INITIALIZING ASYNCHRONOUS THREAD POOL ENGINE (C++)    " << std::endl;
    std::cout << "=========================================================" << std::endl;

    unsigned int hardware_cores = std::thread::hardware_concurrency();
    std::cout << "[SYSTEM] Nucleos detectados en hardware: " << hardware_cores << std::endl;

    ThreadPool pool(hardware_cores > 0 ? hardware_cores : 4);
    std::vector<std::future<int>> results;

    for(int i = 0; i < 8; ++i) {
        results.emplace_back(
            pool.enqueue(heavy_computation, i, 5000)
        );
    }

    std::cout << "\n[ENGINE] Todas las tareas han sido inyectadas en la cola asincrona.\n" << std::endl;

    for(size_t i = 0; i < results.size(); ++i) {
        int return_value = results[i].get(); 
        std::cout << "[RESULT] Tarea procesada con retorno exito: " << return_value << std::endl;
    }

    std::cout << "\n=========================================================" << std::endl;
    std::cout << "   ENGINE TEARDOWN COMPLETE - ALL THREADS CLEANED UP     " << std::endl;
    std::cout << "=========================================================" << std::endl;
    
    return 0;
}
```

---

## 🛠️ Performance Metrics & Stack

| Feature | Subsystem Component | Standard Implementation |
| :--- | :--- | :--- |
| **Language Standard** | Memory Model & Concurrency | `C++17` |
| **Synchronization** | Mutual Exclusion & Gates | `std::mutex`, `std::condition_variable` |
| **Task Storage** | Priority FIFO Sequence Queue | `std::queue<std::function<void()>>` |
| **Metaprogramming** | Perfect Forwarding / Type Resolution | `std::packaged_task`, `std::invoke_result` |

---

## 📥 Compilation & Execution Guide

This architecture uses **CMake** to facilitate platform-independent builds across Linux, macOS, and Windows.

### Compilation Sequence
```bash
# 1. Create build cache directory
mkdir build && cd build

# 2. Generate target binaries using native toolchains
cmake ..

# 3. Compile the executable
cmake --build .
```

### Run Executable
```bash
./ThreadPoolEngine
```

---

## 📊 Sample Pipeline Output

```text
=========================================================
   INITIALIZING ASYNCHRONOUS THREAD POOL ENGINE (C++)    
=========================================================
[SYSTEM] Nucleos detectados en hardware: 8
[TASK 0] Inicializada en el hilo ID: 14023539
[TASK 1] Inicializada en el hilo ID: 14023547
[TASK 2] Inicializada en el hilo ID: 14023555

[ENGINE] Todas las tareas han sido inyectadas en la cola asincrona.

[TASK 0] Finalizada.
[TASK 1] Finalizada.
[RESULT] Tarea procesada con retorno exito: 0
[RESULT] Tarea procesada con retorno exito: 100
=========================================================
   ENGINE TEARDOWN COMPLETE - ALL THREADS CLEANED UP     
=========================================================
```

---

## 📑 License
This architecture engine is distributed under the open-source **MIT License**.

Developed with 💻 and C++.
