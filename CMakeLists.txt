cmake_minimum_required(VERSION 3.15)
project(clang_example C)

set(CMAKE_C_STANDARD 99)

add_executable(clang_example main.c net/blocking_echo_server.c include/constants.h include/blocking_echo_server.h net/noblocking_echo_server.c include/noblocking_echo_server.h net/utils.c include/utils.h net/io_multiplex_echo_server.c include/io_multiplex_echo_server.h net/splice_zero_copy_echo_server.c include/splice_zero_copy_echo_server.h)

target_link_libraries(${PROJECT_NAME} Threads::Threads)
find_package(Threads REQUIRED)