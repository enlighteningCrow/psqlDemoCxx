cmake_minimum_required(VERSION 3.26)
project(psqlDemo)

set(CMAKE_CXX_STANDARD 20)
add_executable(psqlDemo main.cpp)
target_link_libraries(psqlDemo pqxx pq fmt)
