#minimum cmake version required to compile our code
cmake_minimum_required(VERSION 3.4)

# project name
project(multi_file)

# settin constants
#  c++ standard is c++20
set(CMAKE_CXX_STANDARD 20)
#  compilation flags
set(CMAKE_CXX_FLAGS "-Wall -pthread")

# add include/*
include_directories(include/miniz include/logger include/fastflow)

# add debug flag
add_definitions(-DDEBUG)

# adding a subdirectory
add_subdirectory(src)