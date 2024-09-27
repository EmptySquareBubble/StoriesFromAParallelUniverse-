# version 3.11 or later of CMake or needed later for installing GoogleTest
# so let's require it now.
cmake_minimum_required(VERSION 3.11-3.18)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

project(Stories_from_a_parallel_universe
    VERSION 0.1
    DESCRIPTION "Demonstrate power of different execution policies applied on C++ standard library algorithms")

add_executable(two_worlds two_worlds.cpp)

