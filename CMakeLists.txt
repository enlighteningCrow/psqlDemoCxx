cmake_minimum_required(VERSION 3.26)
project(psqlDemo)

set(CMAKE_CXX_STANDARD 20)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Core)

add_executable(psqlDemo main.cpp PostGreSQLHelpers.cpp)
target_link_libraries(psqlDemo pqxx pq fmt Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Core)
