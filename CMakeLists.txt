cmake_minimum_required(VERSION 3.19.2)
project (operationSystems)

set(CMAKE_CXX_STANDARD 14)

add_executable(operationSystems Commands.cpp Commands.h signals.cpp signals.h smash.cpp)
