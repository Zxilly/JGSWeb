cmake_minimum_required(VERSION 3.16)
project(JGSWeb C)

set(CMAKE_C_STANDARD 11)

add_executable(JGSWeb src/jgsweb.c)
target_link_libraries(JGSWeb curl)
add_executable(test test/test.c)
target_link_libraries(test curl)

INCLUDE_DIRECTORIES(/usr/local/include)