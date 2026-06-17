#include <iostream>
#include <chrono>
#include "thread_pool.hpp"

// Simulación de una tarea intensiva de CPU (Cálculo matemático pesado)
int heavy_computation(int id, int matrix_size) {
    std::cout << "[TASK " << id << "] Inicializada en el hilo ID: " 
              << std::this_thread::get_id() << std::endl;
    
    long long accumulator = 0;
    for (int i = 0; i < matrix_size; ++i) {
        for (int j = 0; j < matrix_size; ++j) {
            accumulator += (i * j);
        }
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Simular latencia de I/O
    std::cout << "[TASK " << id << "] Finalizada." << std::endl;
    return id * 100;
}

int main() {
    std::cout << "=========================================================" << std::endl;
    std::cout << "   INITIALIZING ASYNCHRONOUS THREAD POOL ENGINE (C++)    " << std::endl;
    std::cout << "=========================================================" << std::endl;

    // Obtener la cantidad de núcleos físicos del hardware
    unsigned int hardware_cores = std::thread::hardware_concurrency();
    std::cout << "[SYSTEM] Nucleos detectados en hardware: " << hardware_cores << std::endl;

    // Instanciar el pool con el número de hilos óptimo del sistema
    ThreadPool pool(hardware_cores > 0 ? hardware_cores : 4);
    
    // Contenedor para almacenar los objetos "Futuros" de las respuestas asíncronas
    std::vector<std::future<int>> results;

    // Encolar 8 tareas complejas concurrentemente
    for(int i = 0; i < 8; ++i) {
        results.emplace_back(
            pool.enqueue(heavy_computation, i, 5000)
        );
    }

    std::cout << "\n[ENGINE] Todas las tareas han sido inyectadas en la cola asincrona.\n" << std::endl;

    // Bloque principal no bloqueante mientras se procesan los datos
    // Recuperar los resultados mediante el mecanismo de sincronización .get()
    for(size_t i = 0; i < results.size(); ++i) {
        // .get() espera a que el hilo termine de procesar y extrae el valor de retorno de forma segura
        int return_value = results[i].get(); 
        std::cout << "[RESULT] Tarea procesada con retorno exito: " << return_value << std::endl;
    }

    std::cout << "\n=========================================================" << std::endl;
    std::cout << "   ENGINE TEARDOWN COMPLETE - ALL THREADS CLEANED UP     " << std::endl;
    std::cout << "=========================================================" << std::endl;
    
    return 0;
}