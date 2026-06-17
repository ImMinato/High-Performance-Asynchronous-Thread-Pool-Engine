cmake_minimum_required(VERSION 3.12)
project(AsynchronousThreadPoolEngine VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Habilitar hilos en sistemas basados en Unix/Windows
find_package(Threads REQUIRED)

add_executable(ThreadPoolEngine main.cpp)
target_link_libraries(ThreadPoolEngine PRIVATE Threads::Threads)