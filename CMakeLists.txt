cmake_minimum_required(VERSION 3.15)
project(clang_example C)

set(CMAKE_C_STANDARD 99)

add_executable(clang_example main.c net/blocking_echo_server.c net/constants.h net/server.h net/blocking_echo_server.h net/noblocking_echo_server.c net/noblocking_echo_server.h)

target_link_libraries(${PROJECT_NAME} Threads::Threads)
find_package(Threads REQUIRED)