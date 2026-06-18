cmake_minimum_required(VERSION 3.12)
project(AsynchronousThreadPoolEngine VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

find_package(Threads REQUIRED)

add_executable(ThreadPoolEngine main.cpp)
target_link_libraries(ThreadPoolEngine PRIVATE Threads::Threads)

if(MSVC)
    target_compile_options(ThreadPoolEngine PRIVATE /W4)
else()
    target_compile_options(ThreadPoolEngine PRIVATE -Wall -Wextra -Wpedantic)
endif()

install(TARGETS ThreadPoolEngine DESTINATION bin)