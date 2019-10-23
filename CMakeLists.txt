cmake_minimum_required(VERSION 3.13.4)
project(Mars)

set(CMAKE_CXX_STANDARD 14)

add_executable(Mars src/main.cpp)
add_executable(gen_t src/gen_tables.cpp)