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
    
    // Template para aceptar cualquier función y sus argumentos de forma asíncrona
    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args) 
        -> std::future<typename std::invoke_result<F, Args...>::type>;
        
    ~ThreadPool();

private:
    // Lista de hilos trabajadores activos
    std::vector<std::thread> workers;
    
    // Cola de tareas estructurada como funciones genéricas envueltas
    std::queue<std::function<void()>> tasks;
    
    // Primitivas de sincronización para evitar condiciones de carrera (Race Conditions)
    std::mutex queue_mutex;
    std::condition_variable cv;
    bool stop;
};

// Implementación del Constructor
inline ThreadPool::ThreadPool(size_t threads) : stop(false) {
    for(size_t i = 0; i < threads; ++i) {
        workers.emplace_back([this] {
            while(true) {
                std::function<void()> task;
                {
                    // Bloqueo crítico mediante lock de exclusión mutua
                    std::unique_lock<std::mutex> lock(this->queue_mutex);
                    this->cv.wait(lock, [this] { 
                        return this->stop || !this->tasks.empty(); 
                    });
                    
                    if(this->stop && this->tasks.empty()) return;
                    
                    task = std::move(this->tasks.front());
                    this->tasks.pop();
                }
                // Ejecución de la tarea fuera del lock para maximizar concurrencia
                task();
            }
        });
    }
}

// Implementación del método Enqueue (Inyección de tareas concurrentes)
template<class F, class... Args>
auto ThreadPool::enqueue(F&& f, Args&&... args) 
    -> std::future<typename std::invoke_result<F, Args...>::type> {
    
    using return_type = typename std::invoke_result<F, Args...>::type;

    // Encapsulación de la tarea en un puntero inteligente empaquetado (std::packaged_task)
    auto task = std::make_shared<std::packaged_task<return_type()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );
    
    std::future<return_type> res = task->get_future();
    {
        std::unique_lock<std::mutex> lock(queue_mutex);

        // Prevenir inserción de datos si el pool fue destruido
        if(stop) throw std::runtime_error("Enqueue invocado en un ThreadPool detenido.");

        tasks.emplace([task]() { (*task)(); });
    }
    cv.notify_one(); // Notificar a un hilo trabajador disponible
    return res;
}

// Destructor: Garantiza un apagado seguro y limpio (Graceful Teardown)
inline ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;
    }
    cv.notify_all(); // Despertar a todos los hilos para su finalización
    for(std::thread &worker: workers) {
        if(worker.joinable()) worker.join();
    }
}

#endif // THREAD_POOL_HPP